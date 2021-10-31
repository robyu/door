#include <Arduino.h>
#include <unity.h>

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

#include "gpio_pins.h"
#include "doormon.h"

doormon_t doormon;

void test_smoke(void) {
    TEST_ASSERT_EQUAL(1, 1);
}

void test_loop_with_inputs(void) {
    int test_duration_s = 20;
    unsigned long time0_ms;
    
    time0_ms = millis();
    while((int)(millis() - time0_ms) <= (1000 * test_duration_s))
    {
        doormon_update(&doormon);
        
        delay(100); // pause for a bit
        
    }
    TEST_ASSERT_EQUAL(1,1);
}


void setup() {
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);

    UNITY_BEGIN();    // IMPORTANT LINE!
    //RUN_TEST(test_led_builtin_pin_number);

    doormon_init(&doormon,
                 REED_SWITCH0,
                 BTN_DOOR_TEST,
                 LED_DOOR);
}

void loop() {
    RUN_TEST(test_smoke);
    RUN_TEST(test_loop_with_inputs);
    UNITY_END(); // stop unit testing
}
