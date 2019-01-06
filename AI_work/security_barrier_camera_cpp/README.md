//环境搭建ubuntu16.04+openVINO(R5)
cd parking-lot-counter-cpp
./build.sh
//默认使用
./start.sh
//使用摄像头，MQTT
export MQTT_SERVER=localhost:1883
export MQTT_CLIENT_ID=cvservice
//CPU
./security_barrier_camera_demo -i ../../../car_1.bmp -m /opt/intel/computer_vision_sdk/deployment_tools/intel_models/vehicle-license-plate-detection-barrier-0106/FP32/vehicle-license-plate-detection-barrier-0106.xml -m_va /opt/intel/computer_vision_sdk/deployment_tools/intel_models/vehicle-attributes-recognition-barrier-0039/FP32/vehicle-attributes-recognition-barrier-0039.xml -m_lpr /opt/intel/computer_vision_sdk/deployment_tools/intel_models/license-plate-recognition-barrier-0001/FP32/license-plate-recognition-barrier-0001.xml -d CPU
//神经计算棒
./security_barrier_camera_demo -i ../../../car_1.bmp -m /opt/intel/computer_vision_sdk/deployment_tools/intel_models/vehicle-license-plate-detection-barrier-0106/FP16/vehicle-license-plate-detection-barrier-0106.xml -m_va /opt/intel/computer_vision_sdk/deployment_tools/intel_models/vehicle-attributes-recognition-barrier-0039/FP16/vehicle-attributes-recognition-barrier-0039.xml -m_lpr /opt/intel/computer_vision_sdk/deployment_tools/intel_models/license-plate-recognition-barrier-0001/FP16/license-plate-recognition-barrier-0001.xml -d MYRIAD -d_va MYRIAD -d_lpr MYRIAD 
//intelGPU使用FP16版本
./security_barrier_camera_demo -i ../../../car_1.bmp -m /opt/intel/computer_vision_sdk/deployment_tools/intel_models/vehicle-license-plate-detection-barrier-0106/FP16/vehicle-license-plate-detection-barrier-0106.xml -m_va /opt/intel/computer_vision_sdk/deployment_tools/intel_models/vehicle-attributes-recognition-barrier-0039/FP16/vehicle-attributes-recognition-barrier-0039.xml -m_lpr /opt/intel/computer_vision_sdk/deployment_tools/intel_models/license-plate-recognition-barrier-0001/FP16/license-plate-recognition-barrier-0001.xml -d GPU -d_va GPU -d_lpr GPU 
//监视发送到本地服务器的MQTT消息
mosquitto_sub -t 'carid/results'


