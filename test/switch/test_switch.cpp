#include <Arduino.h>
#include <unity.h>
#include "switch.h"

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

#include "gpio_pins.h"

switch_t switch_door0;


int test_duration = 10;

void test_switch_door0(void) {
    unsigned long time0_ms;

    time0_ms = millis();
    
    while((int)(millis() - time0_ms) <= (1000 * test_duration))
    {
        int state = switch_update_state(&switch_door0);
        if (state==0)
        {
            Serial.println("low");
        }
        else
        {
            Serial.println("hi");
        }
        
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

    switch_init(&switch_door0, REED_SWITCH0);
}

void loop() {
    test_switch_door0();
    UNITY_END(); // stop unit testing
}
