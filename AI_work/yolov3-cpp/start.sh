#!/bin/bash
export MQTT_SERVER=localhost:1883
export MQTT_CLIENT_ID=cvservice
cd build/intel64/Release/
./object_detection_demo_yolov3_async -i /home/xs/xs/ncs_objection/test/bus_station_6094_960x540.mp4 -m /home/xs/xs/tensorflow-yolo-v3/yolo_v3.xml -d CPU

