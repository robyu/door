# MQTT BROKER

## Building the Image

First make sure you have an appropriate tag set in the 'image' field, 
e.g. robyu/ryu-mosquitto:rel001.
Then:
> docker compose build

then 
> docker login
> docker push robyu/ryu-mosquitto:rel001


The MQTT broker is implemented as a a docker-compose app. Use start-docker-mqtt.sh
to start the server:
```
./start-docker-mqtt.sh
```

Afterwards, you can confirm that it is running:
```
ryu@ryu-mbp-2020:mqtt-broker$ docker ps
CONTAINER ID   IMAGE                      COMMAND                  CREATED        STATUS        PORTS                                            NAMES
d693acd4fad6   eclipse-mosquitto:2.0.14   "/docker-entrypoint.…"   15 hours ago   Up 15 hours   0.0.0.0:1883->1883/tcp, 0.0.0.0:9001->9001/tcp   mqtt-test-broker_mqttserver_1
```

You can use the script monitor-mqtt.sh to monitor the server. monitor-mqtt splits the window into 3 tmux panes:
 - mqtt server log (via ```docker-compose logs```)
 - a window which subscribes to MQTT messages home/#
 - a window which publishes the sensor reset message

Start it like this:

```
./monitor-mqtt.sh
```

