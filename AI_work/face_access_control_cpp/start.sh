cd ../webservice/server/node-server
gnome-terminal -x bash -c "node ./server.js"
cd ../../front-end
gnome-terminal -x bash -c "npm run dev"
cd ../../
gnome-terminal -x bash -c "sudo ffserver -f ./ffmpeg/server.conf"
cd face-access-control/build
gnome-terminal -x bash -c "export MQTT_SERVER=localhost:1883;export MQTT_CLIENT_ID=cvservice;export FACE_DB=./defaultdb.xml;export FACE_IMAGES=../../webservice/server/node-server/public/profile/;./cvservice 0 2>/dev/null | ffmpeg -f rawvideo -pixel_format bgr24 -video_size vga -i - http://localhost:8090/fac.ffm"


