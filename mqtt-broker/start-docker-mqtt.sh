#!/bin/bash
x=`docker ps | grep mqttserver`
if [[ -n "$x" ]]; then
   echo "stop existing mqttserver"
   #docker stop mqttserver
   docker-compose down
fi
   
#
# --rm remove container after exit
#
# let mosquitto log to stdout, and have docker limit the log size & rotations
# 
echo "running mqttserver"
# docker run --name mqttserver --rm -p 1883:1883 -p 9001:9001 \
#        -v `pwd`/mosquitto.conf:/mosquitto/conf/mosquitto.conf \
#        --log-opt max-size=1m \
#        --log-opt max-file=3 \
#         eclipse-mosquitto:2.0.14 mosquitto -c /mosquitto/conf/mosquitto.conf
docker-compose up&
docker-compose logs -f
