cd build/intel64/Release/
./pedestrian_tracker_demo -i ../../../worker-zone-detection.mp4 -m_det /opt/intel/computer_vision_sdk/deployment_tools/intel_models/person-detection-retail-0013/FP32/person-detection-retail-0013.xml -m_reid /opt/intel/computer_vision_sdk/deployment_tools/intel_models/person-reidentification-retail-0031/FP32/person-reidentification-retail-0031.xml -d_det CPU
cd ../../../
python3 picture_video.py

