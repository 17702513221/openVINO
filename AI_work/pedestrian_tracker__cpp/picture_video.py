# encoding: UTF-8
import glob as gb
import cv2
import os
import shutil
filecount = 0    #计数文件夹下共有多少个小文件
picturecount =0  #计数图片
picture_number =1
for filename in os.listdir('build/intel64/Release'):
    filecount += 1

for i in range(0,filecount-2):    
    videoWriter = cv2.VideoWriter(str(i)+'.mp4', cv2.VideoWriter_fourcc(*'MJPG'), 25, (640,480))
    for picturename in os.listdir('build/intel64/Release/'+str(i)):
        picturecount += 1
    for m in range(1,picturecount+1):
        while (os.path.exists("build/intel64/Release/"+str(i)+"/"+str(picture_number)+".jpg")!=True):
            picture_number += 1
        img  = cv2.imread("build/intel64/Release/"+str(i)+"/"+str(picture_number)+".jpg") 
        img = cv2.resize(img,(640,480))
        videoWriter.write(img)
        picture_number += 1
    picturecount = 0
    picture_number = 1
    shutil.rmtree("build/intel64/Release/"+str(i))    #递归删除文件夹
    

