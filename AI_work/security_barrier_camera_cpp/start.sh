cd build/intel64/Release
export MQTT_SERVER=localhost:1883
export MQTT_CLIENT_ID=cvservice
./security_barrier_camera_demo -i ../../../car_1.bmp -m /opt/intel/computer_vision_sdk/deployment_tools/intel_models/vehicle-license-plate-detection-barrier-0106/FP32/vehicle-license-plate-detection-barrier-0106.xml -m_va /opt/intel/computer_vision_sdk/deployment_tools/intel_models/vehicle-attributes-recognition-barrier-0039/FP32/vehicle-attributes-recognition-barrier-0039.xml -m_lpr /opt/intel/computer_vision_sdk/deployment_tools/intel_models/license-plate-recognition-barrier-0001/FP32/license-plate-recognition-barrier-0001.xml -d CPU

