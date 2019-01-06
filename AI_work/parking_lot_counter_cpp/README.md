//环境搭建ubuntu16.04+openVINO(R5)
cd parking_lot_counter_cpp
./build.sh
//下载测试视频
cd parking_lot_counter_cpp
wget https://github.com/intel-iot-devkit/sample-videos/raw/master/car-detection.mp4
./start.sh
//使用摄像头，MQTT
export MQTT_SERVER=localhost:1883
export MQTT_CLIENT_ID=cvservice
export MQTT_CLIENT_ID=parkinglot1337（更改监控站ID，可不使用）
//CPU
./monitor -m=/opt/intel/computer_vision_sdk/deployment_tools/intel_models/pedestrian-and-vehicle-detector-adas-0001/FP32/pedestrian-and-vehicle-detector-adas-0001.bin -c=/opt/intel/computer_vision_sdk/deployment_tools/intel_models/pedestrian-and-vehicle-detector-adas-0001/FP32/pedestrian-and-vehicle-detector-adas-0001.xml -cc=0.7 -e="b"
//神经计算棒
./monitor -m=/opt/intel/computer_vision_sdk/deployment_tools/intel_models/pedestrian-and-vehicle-detector-adas-0001/FP16/pedestrian-and-vehicle-detector-adas-0001.bin -c=/opt/intel/computer_vision_sdk/deployment_tools/intel_models/pedestrian-and-vehicle-detector-adas-0001/FP16/pedestrian-and-vehicle-detector-adas-0001.xml -b=2 -t=3
//intelGPU使用FP16版本
./monitor -m=/opt/intel/computer_vision_sdk/deployment_tools/intel_models/pedestrian-and-vehicle-detector-adas-0001/FP16/pedestrian-and-vehicle-detector-adas-0001.bin -c=/opt/intel/computer_vision_sdk/deployment_tools/intel_models/pedestrian-and-vehicle-detector-adas-0001/FP16/pedestrian-and-vehicle-detector-adas-0001.xml -b=2 -t=2
//操作说明
-entrance标志控制视频流帧的哪一部分用于计算进入和离开停车场的汽车：
b(后部)l(左)r(右)t(顶部)
置信度设置:-cc=0.6
//监视发送到本地服务器的MQTT消息
mosquitto_sub -t 'parking/counter'


