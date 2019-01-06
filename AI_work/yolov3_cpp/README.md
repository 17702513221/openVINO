//环境搭建ubuntu16.04+openVINO(R4)（
//先生成模型，默认下载官网权重转，实际项目可以使用darknet训练自己的权重
git clone https://github.com/17702513221/tensorflow_tools.git
cd tensorflow-yolo-v3
wget https://pjreddie.com/media/files/yolov3.weights
wget https://raw.githubusercontent.com/nealvis/media/master/traffic_vid/bus_station_6094_960x540.mp4
python3 demo.py --weights_file yolov3.weights --class_names coco.names --input_img Traffic.jpg --output_img out.jpg
cd /opt/intel/computer_vision_sdk/deployment_tools/model_optimizer
sudo python3 mo_tf.py --input_model /home/xs/xs/tensorflow-yolo-v3/yolo_v3.pb --tensorflow_use_custom_operations_config extensions/front/tf/yolo_v3.json --input_shape=[1,416,416,3]
将生成的yolo_v3.xml和yolo_v3.bin复制到本文件夹下
cd /home/xs/inference_engine_samples/intel64/Release
//视频测试：
./object_detection_demo_yolov3_async -i /home/xs/tensorflow_tools/tensorflow-yolo-v3/bus_station_6094_960x540.mp4 -m /home/xs/tensorflow_tools/tensorflow-yolo-v3/yolo_v3.xml -d CPU
//摄像头测试：
./object_detection_demo_yolov3_async -i cam -m /home/xs/tensorflow_tools/tensorflow-yolo-v3/yolo_v3.xml -d CPU
//下载我的开源项目运行：
git clone https://github.com/17702513221/openVINO.git
cd AI_work/yolov3-cpp
./build.sh
//测试(需先用tensorflow-yolo-v3生成模型，测试默认CPU其它需求自行修改)
./start.sh
//监视发送到本地服务器的MQTT消息，发送的是labels的序号，如：person对应0(需先使用新终端开启本地服务器)
mosquitto_sub -t 'yolov3/results'
