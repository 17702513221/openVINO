//ubuntu16.04+OpenVINO(R4)+USB摄像头
//该项目默认服务器与处理后台放在一起，和通过修改实现前后端分离。项目使用Node.js，后台使用C++构建mosquitto服务器。
sudo apt update
sudo apt install ffmpeg
sudo apt install libssl-dev
git clone https://github.com/17702513221/openVINO.git
cd /home/xs/openVINO/AI_work/cvservice
mkdir -p build && cd build
cmake ..
make
//配置环境
sudo apt update
sudo apt install npm nodejs nodejs-dev nodejs-legacy
sudo apt install libzmq3-dev libkrb5-dev
sudo apt install sqlitebrowser
cd /home/xs/openVINO/AI_work/webservice/server
npm install
cd /home/xs/openVINO/AI_work/webservice/front-end
npm install
npm run dist
//运行程序
//1.启动Web服务，包括服务器和前端组件。
cd /home/xs/openVINO/AI_work/webservice/server/node-server
node ./server.js
cd /home/xs/openVINO/AI_work/webservice/front-end
npm run dev
//2.启动ffserver
cd /home/xs/openVINO/AI_work
sudo ffserver -f ./ffmpeg/server.conf
//4.启动cvservice和pipe到ffmpeg：（笔记本自带摄像头有BUG，我使用的是USB摄像头）
cd /home/xs/openVINO/AI_work/face-access-control/build
 export MQTT_SERVER=localhost:1883
 export MQTT_CLIENT_ID=cvservice
 export FACE_DB=./defaultdb.xml
 export FACE_IMAGES=../../webservice/server/node-server/public/profile/
 ./cvservice 0 2>/dev/null | ffmpeg -f rawvideo -pixel_format bgr24 -video_size vga -i - http://localhost:8090/fac.ffm
//5.浏览器打开
http://localhost:8080
//监控MQTT发送的数据
mosquitto_sub -t 'person/seen'
