python版mqtt
//安装依赖
pip3 install paho-mqtt
sudo apt install mosquitto-clients
//webservice/server需先开启
python3 mqtt_client.py
mosquitto_sub -t 'chat'
python3 mqtt_client2.py
mosquitto_sub -t 'test/server'

