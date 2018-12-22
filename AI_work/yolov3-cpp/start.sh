#!/bin/bash
export MQTT_SERVER=localhost:1883
export MQTT_CLIENT_ID=cvservice
cd build/intel64/Release/
#./object_detection_demo_yolov3_async -i cam -m /home/xs/tensorflow_tools/tensorflow-yolo-v3/yolo_v3.xml -d CPU 
./object_detection_demo_yolov3_async -i cam -m /home/xs/tensorflow_tools/tensorflow-yolo-v3/yolo_v3.xml -d CPU 2>/dev/null | ffmpeg -f rawvideo -pixel_format bgr24 -video_size vga -i - http://localhost:8090/fac.ffm
#640x480 vga
#1080 x 720 笔记本
