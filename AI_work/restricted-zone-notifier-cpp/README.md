//环境搭建ubuntu16.04+openVINO(R4)
cd restricted-zone-notifier-cpp
mkdir -p build && cd build
cmake ..
make
//下载测试视频
cd restricted-zone-notifier-cpp
wget https://github.com/intel-iot-devkit/sample-videos/raw/master/worker-zone-detection.mp4
cd build
./monitor -m=/opt/intel/computer_vision_sdk/deployment_tools/intel_models/pedestrian-detection-adas-0002/FP32/pedestrian-detection-adas-0002.bin -c=/opt/intel/computer_vision_sdk/deployment_tools/intel_models/pedestrian-detection-adas-0002/FP32/pedestrian-detection-adas-0002.xml -i=../worker-zone-detection.mp4
//使用摄像头，MQTT
export MQTT_SERVER=localhost:1883
export MQTT_CLIENT_ID=cvservice
export MQTT_CLIENT_ID=zone1337（更改监控站ID，可不使用）
//CPU
./monitor -m=/opt/intel/computer_vision_sdk/deployment_tools/intel_models/pedestrian-detection-adas-0002/FP32/pedestrian-detection-adas-0002.bin -c=/opt/intel/computer_vision_sdk/deployment_tools/intel_models/pedestrian-detection-adas-0002/FP32/pedestrian-detection-adas-0002.xml 
//神经计算棒
./monitor -m=/opt/intel/computer_vision_sdk/deployment_tools/intel_models/pedestrian-detection-adas-0002/FP16/pedestrian-detection-adas-0002.bin -c=/opt/intel/computer_vision_sdk/deployment_tools/intel_models/pedestrian-detection-adas-0002/FP16/pedestrian-detection-adas-0002.xml -b=2 -t=3
//intelGPU使用FP16版本
./monitor -m=/opt/intel/computer_vision_sdk/deployment_tools/intel_models/pedestrian-detection-adas-0002/FP16/pedestrian-detection-adas-0002.bin -c=/opt/intel/computer_vision_sdk/deployment_tools/intel_models/pedestrian-detection-adas-0002/FP16/pedestrian-detection-adas-0002.xml -b=2 -t=2
//操作说明
按C键重新设置检测区，回车确定
//监视发送到本地服务器的MQTT消息
mosquitto_sub -t 'machine/zone'


