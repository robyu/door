#include "Arduino.h"
#include <ESP8266WiFi.h>
#include "gpio_pins.h"
#include "doormon.h"
#include "wifimon.h"

typedef struct
{
    doormon_t doormon;
    wifimon_t wifimon;
} door_t;
door_t door;

void setup()
{
    Serial.begin(115200);
    delay(10); // need this?


    doormon_init(&door.doormon, REED_SWITCH0, BTN_DOOR_TEST, LED_DOOR);
    wifimon_init(&door.wifimon, LED_WIFI, BTN_RESET);


    Serial.println("setup complete");
    Serial.flush();
}



// the loop function runs over and over again forever
void loop() {
    Serial.printf("hello, world\n");
    delay(250); 
    doormon_update(&door.doormon);
    wifimon_update(&door.wifimon);
}

