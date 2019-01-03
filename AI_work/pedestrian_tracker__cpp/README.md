//环境搭建ubuntu16.04+openVINO(R5)（自己根据之前的项目改写的，依赖还没统计，如果前面例子跑通，这个就能运行）
//下载我的开源项目运行：
git clone https://github.com/17702513221/openVINO.git
cd AI_work/pedestrian_tracker_cpp
wget https://github.com/intel-iot-devkit/sample-videos/raw/master/worker-zone-detection.mp4
./build.sh
//测试
./start.sh
//监视发送到本地服务器的MQTT消息：
mosquitto_sub -t 'pedestrian/results'
