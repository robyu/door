#!/bin/bash
docker run -it -p 1883:1883 -p 9001:9001 -v mosquitto.conf:`pwd`/mosquitto.conf   eclipse-mosquitto:2.0.14 mosquitto -c /mosquitto-no-auth.conf
