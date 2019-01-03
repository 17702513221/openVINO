cd build/intel64/Release/
./crossroad_camera_demo -i ../../../worker-zone-detection.mp4 -m /opt/intel/computer_vision_sdk/deployment_tools/intel_models/person-vehicle-bike-detection-crossroad-0078/FP32/person-vehicle-bike-detection-crossroad-0078.xml -m_pa /opt/intel/computer_vision_sdk/deployment_tools/intel_models/person-attributes-recognition-crossroad-0200/FP32/person-attributes-recognition-crossroad-0200.xml -m_reid /opt/intel/computer_vision_sdk/deployment_tools/intel_models/person-reidentification-retail-0079/FP32/person-reidentification-retail-0079.xml -d CPU -d_pa CPU -d_reid CPU

