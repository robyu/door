#include <Arduino.h>
#include <unity.h>
#include "LittleFS.h"
#include "wifimon.h"
#include "gpio_pins.h"

wifimon_t wifimon_state;

void setup() {
    Serial.begin(115200);
    delay(1000); // pause for a bit

    wifimon_init(&wifimon_state,
                 LED_WIFI,
                 BTN_RESET);
    TEST_ASSERT_TRUE(true);

}


void loop() {
    //RUN_TEST(test_write);
    UNITY_END();
}
