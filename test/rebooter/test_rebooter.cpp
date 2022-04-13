#include "rebooter.h"
#include <unity.h>
#include "gpio_pins.h"

rebooter_t rebooter;
int test_duration_s = 20;

void test_button_led(void) {
    int longpress_flag;
    unsigned long time0_ms;

    time0_ms = millis();

    Serial.println("You have " + String(test_duration_s) + " sec to press reset");
    
    while((int)(millis() - time0_ms) <= (1000 * test_duration_s))
    {
        longpress_flag = reboot_update_state(&rebooter);
        if (longpress_flag)
        {
            Serial.println("detected reboot button long-press");
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
    rebooter_init(&rebooter, BTN_RESET, LED_BUILTIN_PCB);
}

void loop() {
    test_button_led();
    UNITY_END(); // stop unit testing
}
