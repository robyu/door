DOOR
----

An MQTT-enabled remote door sensor

We have a detached garage, and occasionally the garage door is left open or re-opens on its own. That's why I built DOOR, a remote garage-door sensor. DOOR monitors the state of the garage door, and sends the garage door via Wi-Fi to my house.

DOOR's hardware is built around an [ESP8266](https://en.wikipedia.org/wiki/ESP8266), a Wi-Fi enabled microcontroller which is compatible with the Arduino toolset. The actual sensor is a reed switch connected to a GPIO pin; a beefy magnet on the garage door activates the reed switch.

![](./readme-images/alert-circuit.png)

The firmware is written in C, using [platformio](https://platformio.org/). It maintains the state of the door and publishes MQTT updates via Wi-Fi to a [Mosquitto server](https://mosquitto.org/). The Mosquitto server forwards the MQTT message to my [AlertBot](https://github.com/robyu/alertbot), which sits in the kitchen and loudly proclaims when the garage door is open.

The state machine is surprisingly complex, primarily because it has to deal with the details of Wi-Fi setup and connection, as well as reset commands.

![](./readme-images/master-state-machine.png)

After developing and testing DOOR on a protoboard, I soldered it onto a breadboard and shoved the whole thing into a cigar box. It ain't pretty, but it works.

![](./readme-images/IMG_4701.JPG)

![](.readme-images/IMG_4700.JPG)

Detailed Documentation

https://docs.google.com/document/d/1rnizVCsxLXGlfpaVHpRxiN8h5cO-BPx0BfrcB0WDp4A/edit?usp=sharing

License
=======

MIT License
