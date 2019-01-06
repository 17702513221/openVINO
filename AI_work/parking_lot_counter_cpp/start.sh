cd build
export MQTT_SERVER=localhost:1883
export MQTT_CLIENT_ID=cvservice
./monitor -m=/opt/intel/computer_vision_sdk/deployment_tools/intel_models/pedestrian-and-vehicle-detector-adas-0001/FP32/pedestrian-and-vehicle-detector-adas-0001.bin -c=/opt/intel/computer_vision_sdk/deployment_tools/intel_models/pedestrian-and-vehicle-detector-adas-0001/FP32/pedestrian-and-vehicle-detector-adas-0001.xml -i=../car-detection.mp4 -cc=0.5 -e="t"

