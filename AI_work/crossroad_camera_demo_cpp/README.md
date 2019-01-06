//ubuntu16.04+OpenVINO(R4)+USB摄像头
sudo apt update
sudo apt install libssl-dev
git clone https://github.com/17702513221/openVINO.git
cd /home/xs/openVINO/AI_work/crossroad_camera_demo_cpp
wget https://github.com/intel-iot-devkit/sample-videos/raw/master/worker-zone-detection.mp4
./build.sh
//运行程序
./start.sh

