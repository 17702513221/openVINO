/*
 * Authors: Mihai Tudor Panu <mihai.tudor.panu@intel.com>
 *          Ron Evans <ron@hybridgroup.com>
 * Copyright (c) 2017 Intel Corporation.
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

// System includes
#include <algorithm>
#include <assert.h>
#include <cassert>
#include <cmath>
#include <exception>
#include <iostream>
#include <limits.h>
#include <map>
#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <syslog.h>
#include <sys/stat.h>
#include <time.h>
#include <vector>

// OpenCV includes
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/video/tracking.hpp>
#include <opencv2/pvl/pvl.hpp>

// MQTT
#include "mqtt.h"

using namespace std;
using namespace cv;
using namespace cv::pvl;

volatile bool running = true;
volatile bool performRegistration = false;

const char* DEFAULT_DB_LOCATION = "./defaultdb.xml";
const char* DEFAULT_THUMBNAIL_PATH = "./thumbs/";
const int FONT = cv::FONT_HERSHEY_PLAIN;
const Scalar GREEN(0, 255, 0);
const Scalar BLUE(255, 0, 0);
const Scalar WHITE(255, 255, 255);

Ptr<FaceDetector> pvlFD;
Ptr<FaceRecognizer> pvlFR;
Mat imgIn;
Mat imgGray;
VideoCapture webcam;

vector<Face> detectedFaces;
vector<Face> recognizedFaces;
vector<int>  personIDs;
vector<int>  confidence;

int cameraNumber = 0;
string databaseLocation;
string thumbnailPath;

string lastTopic;
string lastID;

// 解析传入的命令行参数，以确定使用摄像头ID
// 也处理任何ENV变量
void parseArgs(int argc, const char* argv[]) {
    if (argc > 1) {
        cameraNumber = atoi(argv[1]);
    }

    databaseLocation = std_getenv("FACE_DB");
    if (databaseLocation.empty()) {
        databaseLocation = DEFAULT_DB_LOCATION;
    }

    thumbnailPath = std_getenv("FACE_IMAGES");
    if (thumbnailPath.empty()) {
        thumbnailPath = DEFAULT_THUMBNAIL_PATH;
    }
}

// 使用JSON格式推送MQTT信息
void publishMQTTMessage(const string& topic, const string& id)
{
    // 重复消息不发送
    if (lastTopic == topic && lastID == id) {
        return;
    }

    lastTopic = topic;
    lastID = id;

    string payload = "{\"id\": \"" + id + "\"}";

    mqtt_publish(topic, payload);

    string msg = "MQTT message published to topic: " + topic;
    syslog(LOG_INFO, "%s", msg.c_str());
    syslog(LOG_INFO, "%s", payload.c_str());
}

// 对于“命令/寄存器”的MQTT订阅的信息处理
int handleControlMessages(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    string topic = topicName;
    string msg = "MQTT message received: " + topic;
    syslog(LOG_INFO, "%s", msg.c_str());

    if (topic == "commands/register") {
        performRegistration = true;
    }
    return 1;
}

// 打开摄像头输入
bool openWebcamInput(int cameraNumber) {
    webcam.open(cameraNumber);

    if (!webcam.isOpened())
    {
        syslog(LOG_ERR, "Error: fail to capture video.");
        return false;
    }

    return true;
}

// 从磁盘上的XML文件加载面部数据库
bool loadFaceDB(const string& dbPath) {
    // 判断DB文件是否存在
    struct stat tmpbuffer;
    if (stat(dbPath.c_str(), &tmpbuffer) != 0) {
        syslog(LOG_WARNING, "Unable to locate face DB. Will be created on save.");
        return false;
    }

    // 尝试加载现有的DB文件
    pvlFR = Algorithm::load<FaceRecognizer>(dbPath);
    if (pvlFR == NULL)
    {
        syslog(LOG_ERR, "Error: fail to load face DB.");
        return false;
    }

    pvlFR->setTrackingModeEnabled(true);
    return true;
}

// 为FaceDetect设置参数 (face size, number of faces, ROI, allowed angle/rotation, etc)
bool initFaceDetection() {
    pvlFD = FaceDetector::create();
    if (pvlFD.empty())
    {
        syslog(LOG_ERR, "Error: fail to create PVL face detector.");
        return false;
    }

    pvlFD->setTrackingModeEnabled(true);
    pvlFD->setMaxDetectableFaces(1);

    pvlFR = FaceRecognizer::create();
    if (pvlFR.empty())
    {
        syslog(LOG_ERR, "Error: fail to create PVL face recognizer.");
        return false;
    }

    pvlFR->setTrackingModeEnabled(true);

    loadFaceDB(databaseLocation);

    return true;
}

bool getNextImage() {
    webcam >> imgIn;
    if (imgIn.empty())
    {
        syslog(LOG_ERR, "Error: no input image.");
        return false;
    }

    //准备分析用的灰度图像
    cvtColor(imgIn, imgGray, cv::COLOR_BGR2GRAY);
    if (imgGray.empty())
    {
        syslog(LOG_ERR, "Error: cannot convert input image to gray.");
        return false;
    }

    return true;
}

// 检测任何面部，并存储面部数据
void lookForFaces() {
    detectedFaces.clear();
    personIDs.clear();
    confidence.clear();

    //做面部检测
    pvlFD->detectFaceRect(imgGray, detectedFaces);
}

//尝试识别任何检测到的面孔
void recognizeFaces() {
    if (detectedFaces.size() > 0)
    {
        recognizedFaces.clear();

        int recognizedFaceCount = 0;
        recognizedFaceCount = MIN(static_cast<int>(detectedFaces.size()), pvlFR->getMaxFacesInTracking());

        for (int i = 0; i < recognizedFaceCount; i++)
        {
            recognizedFaces.push_back(detectedFaces[i]);
        }

        pvlFR->recognize(imgGray, recognizedFaces, personIDs, confidence);
        bool saveNeeded = false;

        for (uint i = 0; i < personIDs.size(); i++)
        {
            if (personIDs[i] == FACE_RECOGNIZER_UNKNOWN_PERSON_ID)
            {
                if (performRegistration)
                {
                    int personID = pvlFR->createNewPersonID();
                    pvlFR->registerFace(imgGray, detectedFaces[i], personID, true);

                    publishMQTTMessage("person/registered", to_string(personID));

                    string saveFileName = thumbnailPath+to_string(personID)+".jpg";
                    cv:imwrite(saveFileName.c_str(), imgIn);

                    saveNeeded = true;
                    performRegistration = false;
                } else {
                    publishMQTTMessage("person/seen", "UNKNOWN");
                }
            } else {
                publishMQTTMessage("person/seen", to_string(personIDs[i]));
            }
        }

        if (saveNeeded) {
            pvlFR->save(databaseLocation);
        }
    } else {
        lastTopic.clear();
        lastID.clear();
    }
}

//图像中检测的面部信息在窗口上显示
void displayDetectionInfo()
{
    for (uint i = 0; i < detectedFaces.size(); ++i)
    {
        const Face& face = detectedFaces[i];
        Rect faceRect = face.get<Rect>(Face::FACE_RECT);

        // Draw face rect
        rectangle(imgIn, faceRect, WHITE, 2);
    }
}

//图像中识别的面部信息在窗口上显示
void displayRecognitionInfo()
{
    cv::String str;

    for (uint i = 0; i < detectedFaces.size(); i++)
    {
        const Face& face = detectedFaces[i];
        Rect faceRect = face.get<Rect>(Face::FACE_RECT);

        //draw FR info
        str = (personIDs[i] > 0) ? cv::format("Person: %d(%d)", personIDs[i], confidence[i]) : "UNKNOWN";

        cv::Size strSize = cv::getTextSize(str, FONT, 1.2, 2, NULL);
        cv::Point strPos(faceRect.x + (faceRect.width / 2) - (strSize.width / 2), faceRect.y - 2);
        cv::putText(imgIn, str, strPos, FONT, 1.2, GREEN, 2);
    }
}

// 将BGR24原始格式输出到控制台.
void outputFrame() {
    int i,j;
    unsigned char b, g, r;
    Vec3b pixel;
    for(int j = 0;j < imgIn.rows;j++){
      for(int i = 0;i < imgIn.cols;i++){
          pixel = imgIn.at<Vec3b>(j, i);
          printf("%c%c%c", pixel[0], pixel[1], pixel[2]);
      }
    }
    fflush(stdout);
}

// 显示窗口图像
void display() {
    displayDetectionInfo();
    displayRecognitionInfo();

    outputFrame();
}

int main(int argc, const char* argv[])
{
    syslog(LOG_INFO, "Starting cvservice...");

    parseArgs(argc, argv);

    try
    {
        int result = mqtt_start(handleControlMessages);
        if (result == 0) {
            syslog(LOG_INFO, "MQTT started.");
        } else {
            syslog(LOG_INFO, "MQTT NOT started: have you set the ENV varables?");
        }

        mqtt_connect();
        mqtt_subscribe("commands/register");

        if (!openWebcamInput(cameraNumber)) {
            throw invalid_argument("Invalid camera number or unable to open camera device.");
            return 1;
        }

        if (!initFaceDetection()) {
            throw runtime_error("Unable to initialize face detection or face recognition.");
            return 1;
        }

        while(running)
        {
            if (getNextImage())
            {
                lookForFaces();

                recognizeFaces();

                display();
            }
        }

        mqtt_disconnect();
        mqtt_close();
        return 0;
    }
    catch(const std::exception& error)
    {
        syslog(LOG_ERR, "%s", error.what());
        return 1;
    }
    catch(...)
    {
        syslog(LOG_ERR, "Unknown/internal exception ocurred");
        return 1;
    }
}
