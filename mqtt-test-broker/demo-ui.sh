#!/bin/bash
#

#
# session
#   window
#     pane


# -d do not attach
# -s session name
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
tmux new-session -d -s site -n foo -c "$SCRIPT_DIR"
tmux split-window -v -d
tmux split-window -h -d

tmux send-keys './start-docker-mqtt.sh' Enter 
sleep 3  # give docker server time to start up

tmux select-pane -R
tmux send-keys 'mosquitto_sub -v -t home/#' Enter 

tmux select-pane -D
tmux send-keys 'mosquitto_pub -h 127.0.0.1 -t home/garage/door -m "open"' Enter 

# attach to session
# works for both inside or outside tmux
[ -n "${TMUX:-}" ] &&
    tmux switch-client -t '=site' ||
	tmux attach-session -t '=site'

# -d do not attach
# -t set target window
#    "=site" forces exact match
# -n window name
#tmux new-window -d -t '=site' -n server 
#tmux send-keys -t '=site:=server' 'ls' Enter

#tmux new-window -d -t '=site' -n jekyll
#tmux send-keys -t '=site:=jekyll' 'top' Enter



