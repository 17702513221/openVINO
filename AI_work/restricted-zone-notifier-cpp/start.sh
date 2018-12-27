cd build
export MQTT_SERVER=localhost:1883
export MQTT_CLIENT_ID=cvservice
./monitor -m=/opt/intel/computer_vision_sdk/deployment_tools/intel_models/pedestrian-detection-adas-0002/FP32/pedestrian-detection-adas-0002.bin -c=/opt/intel/computer_vision_sdk/deployment_tools/intel_models/pedestrian-detection-adas-0002/FP32/pedestrian-detection-adas-0002.xml -i=../worker-zone-detection.mp4

