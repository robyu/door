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

switch_t switch_reed0;
switch_t switch_door_btn;
switch_t switch_reset;

int test_duration_s = 10;

void test_switch_reed0(void) {
    String label = "reed0";
    unsigned long time0_ms;

    time0_ms = millis();
    
    while((int)(millis() - time0_ms) <= (1000 * test_duration_s))
    {
        int state = switch_update_state(&switch_reed0);
        if (state==0)
        {
            Serial.println(label + " low");
        }
        else
        {
            Serial.println(label + " hi");
        }
        
        delay(100); // pause for a bit
        
    }
    TEST_ASSERT_EQUAL(1,1);
}

void test_switch_door_btn(void) {
    String label = "door_btn";
    unsigned long time0_ms;

    time0_ms = millis();
    
    while((int)(millis() - time0_ms) <= (1000 * test_duration_s))
    {
        int state = switch_update_state(&switch_door_btn);
        if (state==0)
        {
            Serial.println(label + " low");
        }
        else
        {
            Serial.println(label + " hi");
        }
        
        delay(100); // pause for a bit
        
    }
    TEST_ASSERT_EQUAL(1,1);
}

void test_switch_reset(void) {
    String label = "reset";
    unsigned long time0_ms;

    time0_ms = millis();
    
    while((int)(millis() - time0_ms) <= (1000 * test_duration_s))
    {
        int state = switch_update_state(&switch_reset);
        if (state==0)
        {
            Serial.println(label + " low");
        }
        else
        {
            Serial.println(label + " hi");
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

    switch_init(&switch_reed0, REED_SWITCH0);
    switch_init(&switch_door_btn, BTN_DOOR_TEST);
    switch_init(&switch_reset, BTN_RESET);
}

void loop() {
    test_switch_reed0();
    test_switch_door_btn();
    test_switch_reset();
    UNITY_END(); // stop unit testing
}
