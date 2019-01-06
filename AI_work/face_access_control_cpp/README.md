//ubuntu16.04+OpenVINO(R4)+USB摄像头
//该项目默认服务器与处理后台放在一起，和通过修改实现前后端分离。项目使用Node.js，后台使用C++构建mosquitto服务器。
sudo apt update
sudo apt install ffmpeg
sudo apt install libssl-dev
git clone https://github.com/17702513221/openVINO.git
cd /home/xs/openVINO/AI_work/cvservice
./build.sh
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
./start.sh
//浏览器打开
http://localhost:8080
//监控MQTT发送的数据
mosquitto_sub -t 'person/seen'
