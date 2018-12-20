//安装依赖环境
sudo apt update
sudo apt install npm nodejs nodejs-dev nodejs-legacy
sudo apt install libzmq3-dev libkrb5-dev
sudo apt install libssl-dev
sudo apt-get install doxygen graphviz
git clone https://github.com/17702513221/openVINO.git
cd /home/xs/openVINO/reference_example/paho.mqtt.c
make
make html
sudo make install
sudo ldconfig
cd /home/xs/openVINO/reference_example/people-counter/ieservice
mkdir -p build && cd build
source /opt/intel/computer_vision_sdk/bin/setupvars.sh
cmake ..
make
cd /home/xs/openVINO/reference_example/people-counter/webservice/ui
npm install
cd /home/xs/openVINO/reference_example/people-counter/webservice/server
npm install
sudo apt install ffmpeg
//运行程序
cd /home/xs/openVINO/reference_example/people-counter/webservice/server/node-server
node ./server.js
cd /home/xs/openVINO/reference_example/people-counter/webservice/ui
npm run dev
cd /home/xs/openVINO/reference_example/people-counter
sudo ffserver -f ./ffmpeg/server.conf
cd /home/xs/openVINO/reference_example/people-counter/ieservice/bin/intel64/Release
wget https://raw.githubusercontent.com/nealvis/media/master/traffic_vid/bus_station_6094_960x540.mp4
export MQTT_SERVER=localhost:1884
export MQTT_CLIENT_ID=cvservice
./obj_recognition -i bus_station_6094_960x540.mp4 -m ssd-cpu.xml -l  ssd-cpu.bin -d CPU -t SSD -thresh 0.7 0 2>/dev/null | ffmpeg -v warning -f rawvideo -pixel_format bgr24 -video_size 544x320 -i - http://localhost:8090/fac.ffm
//5.浏览器打开
http://localhost:8080
