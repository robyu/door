#!/bin/bash

SESSION=mqttmon


x=`docker ps | grep mqttserver`
if [[ -z "$x" ]]; then
    echo "Could not detect mqttserver docker"
    echo "Start mqttserver first"
    exit 1
fi

#
# -d do not attach
# -s session name
# -A attach session if it already exists
tmux new-session -A -d -s "$SESSION" 

# -t target session:pane
tmux split-window -d -t "=$SESSION:0" -v 
tmux split-window -d -t "=$SESSION:0" -h 

# pane 0:
# show mosquitto logs 
tmux send-keys -t "2" "docker-compose logs -f" Enter

# pane 1:
# subscribe to MQTT messages
tmux send-keys -t "0" 'mosquitto_sub -v -t home/#' Enter 

# pane 2:
# command for sending reset
tmux select-pane -t "1"
tmux send-keys -t "1" Enter
tmux send-keys -t "1" 'mosquitto_pub -h 127.0.0.1 -t home/garage-sensor/reset/reset-now/set -m "blah"'

tmux attach-session -t "$SESSION"


