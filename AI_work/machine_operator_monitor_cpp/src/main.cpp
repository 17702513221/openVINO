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

#include <fstream>
#include <stdlib.h>
#include <string>
#include <vector>
#include "opencv2/opencv.hpp"
#include "time.h"
#include "CvxText.h"

using namespace std;
using namespace cv;
using namespace dnn;

// OpenCV相关变量
Mat frame, blob, moodBlob, poseBlob;
VideoCapture cap;
int delay = 5;
Net net, moodnet, posenet;
bool moodChecked = false;
bool poseChecked = false;

// 应用参数
String model;
String config;
String sentmodel;
String sentconfig;
String posemodel;
String poseconfig;
int backendId;
int targetId;
int rate;
float confidenceFace;
float confidenceMood;

// 情绪监测相关标志
int angry_timeout;
bool prev_angry = false;
clock_t begin_angry, end_angry;

// 控制后台线程的标志
atomic<bool> keepRunning(true);

// 用于处理UNIX信号的标志
static volatile sig_atomic_t sig_caught = 0;

// mqtt参数
const string topic = "machine/safety";

// WorkerInfo 包含有关机器操作员的信息
struct WorkerInfo
{
    bool watching;
    bool angry;
    bool alert;
};

// currentInfo 包含应用程序跟踪的最新 WorkerInfo 
WorkerInfo currentInfo = {
  false,
  false,
  false,
};

// nextImage为捕获的视频帧提供队列
queue<Mat> nextImage;
String currentPerf;

mutex m, m1, m2;

// TODO: configure time limit for ANGRY and watching
const char* keys =
    "{ help  h     | | Print help message. }"
    "{ device d    | 0 | camera device number. }"
    "{ input i     | | Path to input image or video file. Skip this argument to capture frames from a camera. }"
    "{ model m     | | Path to .bin file of model containing face recognizer. }"
    "{ config c    | | Path to .xml file of model containing network configuration. }"
    "{ faceconf fc  | 0.5 | Confidence factor for face detection required. }"
    "{ moodconf mc  | 0.5 | Confidence factor for emotion detection required. }"
    "{ sentmodel sm     | | Path to .bin file of sentiment model. }"
    "{ sentconfig sc    | | Path to a .xml file of sentimen model containing network configuration. }"
    "{ posemodel pm     | | Path to .bin file of head pose model. }"
    "{ poseconfig pc    | | Path to a .xml file of head pose model containing network configuration. }"
    "{ backend b    | 0 | Choose one of computation backends: "
                        "0: automatically (by default), "
                        "1: Halide language (http://halide-lang.org/), "
                        "2: Intel's Deep Learning Inference Engine (https://software.intel.com/openvino-toolkit), "
                        "3: OpenCV implementation }"
    "{ target t     | 0 | Choose one of target computation devices: "
                        "0: CPU target (by default), "
                        "1: OpenCL, "
                        "2: OpenCL fp16 (half-float precision), "
                        "3: VPU }"
    "{ rate r      | 1 | number of seconds between data updates to MQTT server. }"
    "{ angry a     | 5 | number of seconds during which the operator has been angrily operating the machine. }";

//中文显示
static int ToWchar(char* &src, wchar_t* &dest, const char *locale = "zh_CN.utf8")
{
    if (src == NULL) {
        dest = NULL;
        return 0;
    }

    // 根据环境变量设置locale
    setlocale(LC_CTYPE, locale);

    // 得到转化为需要的宽字符大小
    int w_size = mbstowcs(NULL, src, 0) + 1;

    // w_size = 0 说明mbstowcs返回值为-1。即在运行过程中遇到了非法字符(很有可能使locale
    // 没有设置正确)
    if (w_size == 0) {
        dest = NULL;
        return -1;
    }

    //wcout << "w_size" << w_size << endl;
    dest = new wchar_t[w_size];
    if (!dest) {
        return -1;
    }

    int ret = mbstowcs(dest, src, strlen(src)+1);
    if (ret <= 0) {
        return -1;
    }
    return 0;
}
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

// getCurrentInfo 返回应用程序的最新WorkerInfo
WorkerInfo getCurrentInfo() {
    m2.lock();
    WorkerInfo rtn;
    rtn = currentInfo;
    m2.unlock();

    return rtn;
}

// updateInfo 将应用程序的当前WorkerInfo更新为最新检测到的值 
void updateInfo(WorkerInfo info) {
    m2.lock();
    currentInfo.watching = info.watching;
    currentInfo.angry = info.angry;
    currentInfo.alert = info.alert;
    m2.unlock();
}

// resetInfo 为应用程序重置当前WorkerInfo.
void resetInfo() {
    m2.lock();
    currentInfo.watching = false;
    currentInfo.angry = false;
    m2.unlock();
}

// getCurrentPerf返回一个显示字符串，其中包含推理引擎的最新性能统计数据
string getCurrentPerf() {
    string rtn;
    m1.lock();
    rtn = currentPerf;
    m1.unlock();

    return rtn;
}

// savePerformanceInfo使用推理引擎的最新性能统计数据设置显示字符串
void savePerformanceInfo() {
    m1.lock();

    vector<double> faceTimes, moodTimes, poseTimes;
    double freq = getTickFrequency() / 1000;
    double t1 = net.getPerfProfile(faceTimes) / freq;
    double t,t2, t3;

    if (moodChecked) {
        t2 = moodnet.getPerfProfile(moodTimes) / freq;
    }

    if (poseChecked) {
        t3 = posenet.getPerfProfile(poseTimes) / freq;
    } 
    t=t1+t2+t3;

    string label = format("%.2fms", t);

    currentPerf = label;

    m1.unlock();
}

// 使用JSON推送MQTT消息
void publishMQTTMessage(const string& topic, const WorkerInfo& info)
{
    ostringstream s;
    s << "{\"watching\": \"" << info.watching << "\",";
    s << "\"angry\": \"" << info.angry << "\"}";
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

// 由工作线程调用的函数来处理下一个可用的视频帧.
void frameRunner() {
    while (keepRunning.load()) {
        Mat next = nextImageAvailable();
        if (!next.empty()) {
            // 根据人脸检测模型的要求，转换为4d矢量，进行人脸检测
            blobFromImage(next, blob, 1.0, Size(672, 384));
            net.setInput(blob);
            Mat prob = net.forward();

            // 获得面孔
            vector<Rect> faces;
            // 机器操作员标志
            bool watching = false;
            bool angry = false;
            bool alert = false;
            float* data = (float*)prob.data;
            for (size_t i = 0; i < prob.total(); i += 7)
            {
                float confidence = data[i + 2];
                if (confidence > confidenceFace)
                {
                    int left = (int)(data[i + 3] * frame.cols);
                    int top = (int)(data[i + 4] * frame.rows);
                    int right = (int)(data[i + 5] * frame.cols);
                    int bottom = (int)(data[i + 6] * frame.rows);
                    int width = right - left + 1;
                    int height = bottom - top + 1;

                    faces.push_back(Rect(left, top, width, height));
                }
            }

            // 检测操作员是否在监视机器
            for(auto const& r: faces) {
                // 确保脸部直接完全在主Mat内
                if ((r & Rect(0, 0, next.cols, next.rows)) != r) {
                    continue;
                }

                // 读取检测到的面部
                Mat face = next(r);

                // posenet输出和包含推理数据的输出层列表
                std::vector<Mat> outs;
                std::vector<String> names{"angle_y_fc", "angle_p_fc", "angle_r_fc"};

                // 改为4D向量，并通过神经网络处理
                blobFromImage(face, poseBlob, 1.0, Size(60, 60));
                posenet.setInput(poseBlob);
                posenet.forward(outs, names);
                poseChecked = true;

                // 改为4D向量，并通过感官神经网络传播
                blobFromImage(face, moodBlob, 1.0, Size(64, 64));
                moodnet.setInput(moodBlob);
                Mat prob = moodnet.forward();
                moodChecked = true;

                // 操作员正在观察他们的头部是否相对于架子倾斜45度以内
                if ( (outs[0].at<float>(0) > -22.5) && (outs[0].at<float>(0) < 22.5) &&
                     (outs[1].at<float>(0) > -22.5) && (outs[1].at<float>(0) < 22.5) ) {
                     watching = true;
                }

                if (watching) {
                    // flatten the result from [1, 5, 1, 1] to [1, 5]
                    Mat flat = prob.reshape(1, 5);
                    // 在返回的情绪列表中找到最大值
                    Point maxLoc;
                    double confidence;
                    minMaxLoc(flat, 0, &confidence, 0, &maxLoc);
                    if (confidence > static_cast<double>(confidenceMood)) {
                        if (maxLoc.y == 4) {
                            angry = true;
                            // 如果操作员在重新启动定时器之前没有生气
                            if (!prev_angry) {
                                begin_angry = clock();
                            }
                        }
                    }
                }
            }

            // 操作员数据
            WorkerInfo info;
            info.watching = watching;
            info.angry = angry;
            info.alert = alert;

            if (watching && angry) {
                end_angry = clock();
                double elapsed_secs = double(end_angry - begin_angry) / CLOCKS_PER_SEC;
                if (elapsed_secs > static_cast<double>(angry_timeout)) {
                    info.alert = true;
                }
            }

            updateInfo(info);
            savePerformanceInfo();

            // 记住以前的愤怒
            prev_angry = angry;
        }
    }

    cout << "Video processing thread stopped" << endl;
}

// 由工作线程调用的函数来处理MQTT更新. 在更新之间暂停几秒.
void messageRunner() {
    while (keepRunning.load()) {
        WorkerInfo info = getCurrentInfo();
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
    confidenceFace = parser.get<float>("faceconf");
    confidenceMood = parser.get<float>("moodconf");

    angry_timeout = parser.get<int>("angry");

    sentmodel = parser.get<String>("sentmodel");
    sentconfig = parser.get<String>("sentconfig");

    posemodel = parser.get<String>("posemodel");
    poseconfig = parser.get<String>("poseconfig");

    // 连接 MQTT messaging
    int result = mqtt_start(handleMQTTControlMessages);
    if (result == 0) {
        syslog(LOG_INFO, "MQTT started.");
    } else {
        syslog(LOG_INFO, "MQTT NOT started: have you set the ENV varables?");
    }

    mqtt_connect();

    //设置中文格式
    CvxText text("../simhei.ttf"); //指定字体
    cv::Scalar size1{ 20, 0.5, 0.1, 0 }; // (字体大小, 无效的, 字符间距, 无效的 }
    text.setFont(nullptr, &size1, nullptr, 0);

    // 打开 face model
    net = readNet(model, config);
    net.setPreferableBackend(backendId);
    net.setPreferableTarget(targetId);

    // 打开 mood model
    moodnet = readNet(sentmodel, sentconfig);
    moodnet.setPreferableBackend(backendId);
    moodnet.setPreferableTarget(targetId);

    // 打开 mood model
    posenet = readNet(posemodel, poseconfig);
    posenet.setPreferableBackend(backendId);
    posenet.setPreferableTarget(targetId);

    // 打开视频资源
    if (parser.has("input")) {
        cap.open(parser.get<String>("input"));

        // 也调整延迟，以便视频回放匹配文件中FPS的数量
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

    // 开启工作线程
    thread t1(frameRunner);
    thread t2(messageRunner);

    // 读取视频输入数据
    for (;;) {
        cap.read(frame);

        if (frame.empty()) {
            keepRunning = false;
            cerr << "ERROR! blank frame grabbed\n";
            break;
        }

        addImage(frame);

        string label = getCurrentPerf();
        char* str1 = (char *)"推理时间：";
        wchar_t *w_str1;
        ToWchar(str1,w_str1);
        text.putText(frame, w_str1, cv::Point(550,20), cv::Scalar(0, 0, 0));
        putText(frame, label, Point(650, 20), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 0, 0));

        WorkerInfo info = getCurrentInfo();
        char* str2 = (char *)"操作者人数：";
        wchar_t *w_str2;
        ToWchar(str2,w_str2);
        text.putText(frame, w_str2, cv::Point(0,20), cv::Scalar(0, 0, 0));
        label = format(" %d", info.watching);
        putText(frame, label, Point(100, 20), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 0, 0));

        if (!info.watching) {
            char* str = (char *)"警告：没注视机器";
            wchar_t *w_str;
            ToWchar(str,w_str);
            text.putText(frame, w_str, cv::Point(0,40), cv::Scalar(255, 0, 0));
        }

        if (info.angry) {
            char* str = (char *)"警告：情绪生气";
            wchar_t *w_str;
            ToWchar(str,w_str);
            text.putText(frame, w_str, cv::Point(0,60), cv::Scalar(255, 0, 0));
        }

        imshow("Machine Operator Monitor", frame);

        if (waitKey(delay) >= 0 || sig_caught) {
            cout << "Attempting to stop background threads" << endl;
            keepRunning = false;
            break;
        }
    }

    // 等待线程结束
    t1.join();
    t2.join();

    // 断开连接MQTT messaging
    mqtt_disconnect();
    mqtt_close();

    return 0;
}
