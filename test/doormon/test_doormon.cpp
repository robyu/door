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
    
    doormon_init(&doormon,
                 REED_SWITCH0,
                 BTN_DOOR_TEST,
                 LED_DOOR);

    time0_ms = millis();
    while((int)(millis() - time0_ms) <= (1000 * test_duration_s))
    {
        doormon_update(&doormon);
        
        delay(100); // pause for a bit
        
    }
    TEST_ASSERT_EQUAL(1,1);
}

void test_state_machine(void) {
    int max_num_loops = 50;
    unsigned long delay_ms = 50;
    unsigned long transition_wait_ms = 100;
    int loop_cnt = 0;
    doormon_state_t ds;

    doormon_init(&doormon,
                 REED_SWITCH0,
                 BTN_DOOR_TEST,
                 LED_DOOR);

    // reduce doormon's transition duration times for testing
    doormon.transition_wait_ms = transition_wait_ms;

    while(loop_cnt < max_num_loops)
    {
        ds = doormon_update(&doormon);
        Serial.println(String(loop_cnt) + ": " + String(doormon_get_curr_state_as_string(&doormon)) +
                       " @ " + String(doormon_get_elapsed_state_time_ms(&doormon)));
        delay(delay_ms);

        if (loop_cnt==0)
        {
            TEST_ASSERT_EQUAL(ds, DM_DOOR_CLOSED);
            TEST_ASSERT_EQUAL(doormon_get_test_flag(&doormon), 0);
        }
        
        if (loop_cnt==10)
        {
            doormon_set_test_flag(&doormon, 1);
        }
        if (loop_cnt==11)
        {
            TEST_ASSERT_EQUAL(ds, DM_DOOR_OPENING);
        }
        if (loop_cnt==15)
        {
            TEST_ASSERT_EQUAL(ds, DM_DOOR_OPEN);
            TEST_ASSERT_TRUE(doormon_get_elapsed_state_time_ms(&doormon) >= 50);
    
        }
        if (loop_cnt==30)
        {
            TEST_ASSERT_EQUAL(ds, DM_DOOR_OPEN);
            doormon_set_test_flag(&doormon, 0);
        }
        if (loop_cnt==31)
        {
            TEST_ASSERT_EQUAL(ds, DM_DOOR_CLOSING);
        }
        if (loop_cnt==35)
        {
            TEST_ASSERT_EQUAL(ds, DM_DOOR_CLOSED);
        }

        
        loop_cnt++;
    }
    
}

void test_extended_run(void) {
    int test_duration_s = 20;
    doormon_state_t ds;
    unsigned long time0_ms;
    
    doormon_init(&doormon,
                 REED_SWITCH0,
                 BTN_DOOR_TEST,
                 LED_DOOR);

    // open the door
    doormon_set_test_flag(&doormon, 1);
    
    time0_ms = millis();

    ds = doormon_update(&doormon);  // initial state

    // loop for a while and make sure it doesnt change state
    while((int)(millis() - time0_ms) <= (1000 * test_duration_s))
    {
        ds = doormon_update(&doormon);
        TEST_ASSERT_TRUE((ds==DM_DOOR_OPENING) || (ds==DM_DOOR_OPEN));
        
        delay(50); // pause for a bit
        
    }
}

void setup() {
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);

    UNITY_BEGIN();    // IMPORTANT LINE!

}

void loop() {
    RUN_TEST(test_smoke);
    //RUN_TEST(test_loop_with_inputs);
    RUN_TEST(test_state_machine);
    RUN_TEST(test_extended_run);
    UNITY_END(); // stop unit testing
}
