/*
* Copyright (c) 2018 Intel Corporation.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
* LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
* OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
* WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

// std includes
#include <iostream>
#include <stdio.h>
#include <thread>
#include <queue>
#include <map>
#include <atomic>
#include <csignal>
#include <ctime>
#include <mutex>
#include <syslog.h>

// OpenCV includes
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>

// MQTT
#include "mqtt.h"

using namespace std;
using namespace cv;
using namespace dnn;

// OpenCV相关变量
Mat frame, blob, im_info;
VideoCapture cap;
int delay = 5;
Net net;

// 应用参数
String model;
String config;
int backendId;
int targetId;
int rate;
float confidenceFactor;

// 应用程序相关标志
const string selector = "Assembly Selection";
// 选定装配线区域
Rect area;

// 控制后台线程的标志
atomic<bool> keepRunning(true);

// 用于处理UNIX信号的标志
static volatile sig_atomic_t sig_caught = 0;

// mqtt参数
const string topic = "machine/zone";

// AssemblyInfo包含关于装配线的信息
struct AssemblyInfo
{
    bool safe;
    bool alert;
};

// currentInfo包含应用程序跟踪的最新AssemblyInfo
AssemblyInfo currentInfo = {
  true,
  false,
};

// nextImage为捕获的视频帧提供队列
queue<Mat> nextImage;
String currentPerf;

mutex m, m1, m2;

const char* keys =
    "{ help     | | Print help message. }"
    "{ device d    | 0 | camera device number. }"
    "{ input i     | | Path to input image or video file. Skip this argument to capture frames from a camera. }"
    "{ model m     | | Path to .bin file of model containing face recognizer. }"
    "{ config c    | | Path to .xml file of model containing network configuration. }"
    "{ factor f    | 0.5 | Confidence factor required. }"
    "{ backend b   | 0 | Choose one of computation backends: "
                        "0: automatically (by default), "
                        "1: Halide language (http://halide-lang.org/), "
                        "2: Intel's Deep Learning Inference Engine (https://software.intel.com/openvino-toolkit), "
                        "3: OpenCV implementation }"
    "{ target t    | 0 | Choose one of target computation devices: "
                        "0: CPU target (by default), "
                        "1: OpenCL, "
                        "2: OpenCL fp16 (half-float precision), "
                        "3: VPU }"
    "{ rate r      | 1 | number of seconds between data updates to MQTT server. }"
    "{ pointx x    | 0 | X coordinate of the top left point of assembly area on camera feed. }"
    "{ pointy y    | 0 | Y coordinate of the top left point of assembly area on camera feed. }"
    "{ width w     | 0 | Width of the assembly area in pixels. }"
    "{ height h    | 0 | Height of the assembly area in pixels. }";


// nextImageAvailable以线程安全的方式从队列返回下一个图像
Mat nextImageAvailable() {
    Mat rtn;
    m.lock();
    if (!nextImage.empty()) {
        rtn = nextImage.front();
        nextImage.pop();
    }
    m.unlock();

    return rtn;
}

// addImage以线程安全的方式向队列添加图像
void addImage(Mat img) {
    m.lock();
    if (nextImage.empty()) {
        nextImage.push(img);
    }
    m.unlock();
}

// getCurrentInfo返回应用程序的最新AssemblyInfo
AssemblyInfo getCurrentInfo() {
    m2.lock();
    AssemblyInfo info;
    info = currentInfo;
    m2.unlock();

    return info;
}

// updateInfo将应用程序的当前AssemblyInfo更新为最新检测到的值
void updateInfo(AssemblyInfo info) {
    m2.lock();
    currentInfo.safe = info.safe;
    currentInfo.alert = info.alert;
    m2.unlock();
}

// resetInfo为应用程序重置当前AssemblyInfo
void resetInfo() {
    m2.lock();
    currentInfo.safe = false;
    currentInfo.alert = false;
    m2.unlock();
}

// getCurrentPerf返回一个显示字符串，其中包含推理引擎的最新性能统计数据
string getCurrentPerf() {
    string perf;
    m1.lock();
    perf = currentPerf;
    m1.unlock();

    return perf;
}

// savePerformanceInfo使用推理引擎的最新性能统计数据设置显示字符串
void savePerformanceInfo() {
    m1.lock();

    vector<double> times;
    double freq = getTickFrequency() / 1000;
    double t = net.getPerfProfile(times) / freq;

    string label = format("Person inference time: %.2f ms", t);

    currentPerf = label;

    m1.unlock();
}

// 使用JSON格式发布MQTT消息
void publishMQTTMessage(const string& topic, const AssemblyInfo& info)
{
    ostringstream s;
    s << "{\"Safe\": \"" << info.safe << "\"}";
    string payload = s.str();

    mqtt_publish(topic, payload);

    string msg = "MQTT message published to topic: " + topic;
    syslog(LOG_INFO, "%s", msg.c_str());
    syslog(LOG_INFO, "%s", payload.c_str());
}

// 用于MQTT订阅的任何所需控制信道主题的消息处理程序
int handleMQTTControlMessages(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    string topic = topicName;
    string msg = "MQTT message received: " + topic;
    syslog(LOG_INFO, "%s", msg.c_str());

    return 1;
}

// 由工作线程调用的函数来处理下一个可用的视频帧
void frameRunner() {
    while (keepRunning.load()) {
        Mat next = nextImageAvailable();
        if (!next.empty()) {
            // 根据人脸检测模型的要求，转换为4d矢量，进行人脸检测
            blobFromImage(next, blob, 1.0, Size(672, 384));
            net.setInput(blob);
            Mat result = net.forward();

            // 被探测到的人
            vector<Rect> persons;
            // 装配线区域标志
            bool safe = true;
            bool alert = false;

            float* data = (float*)result.data;
            for (size_t i = 0; i < result.total(); i += 7)
            {
                float confidence = data[i + 2];
                if (confidence > confidenceFactor)
                {
                    int left = (int)(data[i + 3] * frame.cols);
                    int top = (int)(data[i + 4] * frame.rows);
                    int right = (int)(data[i + 5] * frame.cols);
                    int bottom = (int)(data[i + 6] * frame.rows);
                    int width = right - left + 1;
                    int height = bottom - top + 1;

                    persons.push_back(Rect(left, top, width, height));
                }
            }

            // 检测标记区域是否有人
            for(auto const& r: persons) {
                // 确保person rect完全位于主Mat内部
                if ((r & Rect(0, 0, next.cols, next.rows)) != r) {
                    continue;
                }

                // 如果该人员不在被监控的集合区域内，他们安全
                // 否则，我们需要触发警报
                if ((r & area) == r) {
                    alert = true;
                    safe = false;
                }
            }

            // 操作员数据
            AssemblyInfo info;
            info.safe = safe;
            info.alert = alert;

            updateInfo(info);
            savePerformanceInfo();
        }
    }

    cout << "Video processing thread stopped" << endl;
}

// 由工作线程调用的函数来处理MQTT更新. 在更新前停顿几秒.
void messageRunner() {
    while (keepRunning.load()) {
        AssemblyInfo info = getCurrentInfo();
        publishMQTTMessage(topic, info);
        this_thread::sleep_for(chrono::seconds(rate));
    }

    cout << "MQTT sender thread stopped" << endl;
}

// 主线程的信号处理程序
void handle_sigterm(int signum)
{
    /* 我们只在这里处理SIGTERM和SIGKILL */
    if (signum == SIGTERM) {
        cout << "Interrupt signal (" << signum << ") received" << endl;
        sig_caught = 1;
    }
}

int main(int argc, char** argv)
{
    // 解析命令参数
    CommandLineParser parser(argc, argv, keys);
    parser.about("Use this script to using OpenVINO.");
    if (argc == 1 || parser.has("help"))
    {
        parser.printMessage();

        return 0;
    }

    model = parser.get<String>("model");
    config = parser.get<String>("config");
    backendId = parser.get<int>("backend");
    targetId = parser.get<int>("target");
    rate = parser.get<int>("rate");
    confidenceFactor = parser.get<float>("factor");

    area.x = parser.get<int>("pointx");
    area.y = parser.get<int>("pointy");
    area.width = parser.get<int>("width");
    area.height = parser.get<int>("height");

    // 连接MQTT messaging
    int result = mqtt_start(handleMQTTControlMessages);
    if (result == 0) {
        syslog(LOG_INFO, "MQTT started.");
    } else {
        syslog(LOG_INFO, "MQTT NOT started: have you set the ENV varables?");
    }

    mqtt_connect();

    // 打开face model
    net = readNet(model, config);
    net.setPreferableBackend(backendId);
    net.setPreferableTarget(targetId);

    // 打开摄像头输入源
    if (parser.has("input")) {
        cap.open(parser.get<String>("input"));

        // 还调整延迟，以便视频回放匹配文件中FPS的数量
        double fps = cap.get(CAP_PROP_FPS);
        delay = 1000/fps;
    }
    else
        cap.open(parser.get<int>("device"));

    if (!cap.isOpened()) {
        cerr << "ERROR! Unable to open video source\n";
        return -1;
    }

    // 寄存器SIGTERM信号处理器
    signal(SIGTERM, handle_sigterm);

    // 开始工作线程
    thread t1(frameRunner);
    thread t2(messageRunner);

    // 读取视屏输入数据
    for (;;) {
        cap.read(frame);

        if (frame.empty()) {
            keepRunning = false;
            cerr << "ERROR! blank frame grabbed\n";
            break;
        }

        // 如果给出负数，则默认为帧的开始
        if (area.x < 0 || area.y < 0) {
            area.x = 0;
            area.y = 0;
        }

        // 如果默认值是给定值还是负值，我们将默认为整个图片
        if (area.width <= 0) {
            area.width = frame.cols;
        }

        if (area.height <= 0) {
            area.height = frame.rows;
        }

        int keyPressed = waitKey(delay);
        // 'c' key pressed
        if (keyPressed == 99) {
            // 给操作员改变区域的机会
            // 从左上角选择矩形，不要显示十字线
            namedWindow(selector);
            area = selectROI(selector, frame, true, false);
            cout << "Assembly Area Selection: -x=" << area.x << " -y=" << area.y << " -h=" << area.height << " -w=" << area.width << endl;
            destroyWindow(selector);
        } else if (keyPressed == 27) {
            cout << "Attempting to stop background threads" << endl;
            keepRunning = false;
            break;
        }

        // 绘制区域矩形
        rectangle(frame, area, CV_RGB(255,0,0));

        addImage(frame);

        string label = getCurrentPerf();
        putText(frame, label, Point(0, 15), FONT_HERSHEY_SIMPLEX, 0.5, CV_RGB(255, 255, 255));

        AssemblyInfo info = getCurrentInfo();
        label = format("Worker Safe: %s", info.safe ? "true" : "false");
        putText(frame, label, Point(0, 40), FONT_HERSHEY_SIMPLEX, 0.5, CV_RGB(255, 255, 255));

        if (info.alert) {
            string warning;
            warning = format("HUMAN IN ASSEMBLY AREA: PAUSE THE MACHINE!");
            putText(frame, warning, Point(0, 120), FONT_HERSHEY_SIMPLEX, 0.5, CV_RGB(255, 0, 0), 2);
        }

        imshow("Restricted Zone Notifier", frame);

        if (sig_caught) {
            cout << "Attempting to stop background threads" << endl;
            keepRunning = false;
            break;
        }
    }

    // 等待线程结束
    t1.join();
    t2.join();
    cap.release();

    // 关闭连接 MQTT messaging
    mqtt_disconnect();
    mqtt_close();

    return 0;
}
