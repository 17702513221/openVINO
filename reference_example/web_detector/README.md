//环境搭建在ubuntu16.04+OpenVINO(R3)
sudo apt update
sudo apt install ffmpeg
git clone https://github.com/17702513221/openVINO.git
//构建程序（测试版）
cd /home/xs/openVINO/reference_example/web_detector/application
source env.sh
mkdir -p build && cd build
cmake ..
make
//构建程序（web显示版）
cd /home/xs/openVINO/reference_example/web_detector/application
source env.sh
mkdir -p build && cd build
cmake ..
make CXX_DEFINES=-DUI_OUTPUT
//运行该应用程序
cd /home/xs/openVINO/reference_example/web_detector/build
//在CPU上运行
./web_detector -d CPU -m ../resources/ssd-cpu.xml -l ../resources/labels.txt
//在神经计算棒上运行
./web_detector -d MYRIAD -m ../resources/ssd-ncs.xml -l ../resources/labels.txt
//在浏览器上显示结果
google-chrome  --user-data-dir=$HOME/.config/google-chrome/Web_detector --new-window --allow-file-access-from-files --allow-file-access --allow-cross-origin-auth-prompt index.html
//查询摄像头设备号
ls /dev/video*
//修改conf.txt测试摄像头或视频
/dev/video0 person
../resources/bus_station_6094_960x540.mp4 person
