# encoding: utf-8
 
import paho.mqtt.client as mqtt
 
HOST = "127.0.0.1"
PORT = 1883
 
def test():
    client = mqtt.Client()
    client.connect(HOST, PORT, 60)
    client.publish("chat","hello liefyuan",2) # 发布一个主题为'chat',内容为‘hello liefyuan’的信息
    client.loop_forever()
 
if __name__ == '__main__':
    test()

