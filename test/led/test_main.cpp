#include <Arduino.h>
#include <unity.h>
#include "utils.h"

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

#include "gpio_pins.h"

void test_led_door_off(void) {
    utils_set_led(LED_DOOR, 0);
    TEST_ASSERT_EQUAL(HIGH, digitalRead(LED_DOOR));
}

void test_led_door_on(void) {
    utils_set_led(LED_DOOR, 1);
    TEST_ASSERT_EQUAL(LOW, digitalRead(LED_DOOR));
}

void test_led_builtin_pcb_off(void) {
    utils_set_led(LED_BUILTIN_PCB, 0);
    TEST_ASSERT_EQUAL(HIGH, digitalRead(LED_BUILTIN_PCB));
}

void test_led_builtin_pcb_on(void) {
    utils_set_led(LED_BUILTIN_PCB, 1);
    TEST_ASSERT_EQUAL(LOW, digitalRead(LED_BUILTIN_PCB));
}

void test_led_wifi_on(void) {
    //digitalWrite(LED_WIFI, HIGH);
    utils_set_led(LED_WIFI, 1);
    TEST_ASSERT_EQUAL(HIGH, digitalRead(LED_WIFI));
}

void test_led_wifi_off(void) {
    utils_set_led(LED_WIFI, 0);
    //digitalWrite(LED_WIFI, LOW);
    TEST_ASSERT_EQUAL(LOW, digitalRead(LED_WIFI));
}

void setup() {
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);

    UNITY_BEGIN();    // IMPORTANT LINE!
    //RUN_TEST(test_led_builtin_pin_number);

    pinMode(LED_DOOR, OUTPUT);
    pinMode(LED_BUILTIN_PCB, OUTPUT);
    pinMode(LED_WIFI, OUTPUT);
}

int wait_ms = 10000;

// void set_all_low(void)
// {
//     digitalWrite(LED_WIFI, LOW);
//     digitalWrite(LED_DOOR, LOW);
//     digitalWrite(LED_BUILTIN_PCB, LOW);
// }

// void set_all_high(void)
// {
//     digitalWrite(LED_WIFI, HIGH);
//     digitalWrite(LED_DOOR, HIGH);
//     digitalWrite(LED_BUILTIN_PCB, HIGH);
// }

void set_all_leds(int onoff)
{
    utils_set_led(LED_WIFI, onoff);
    utils_set_led(LED_DOOR, onoff);
    utils_set_led(LED_BUILTIN_PCB, onoff);
}


void loop() {
    Serial.flush();

    Serial.println("set all LEDs on");
    set_all_leds(1);
    delay(wait_ms);

    Serial.println("set all LEDs off");
    set_all_leds(0);
    delay(wait_ms);

    Serial.println("LED DOOR ON");
    RUN_TEST(test_led_door_on);
    delay(wait_ms);

    Serial.println("LED DOOR OFF");
    RUN_TEST(test_led_door_off);
    delay(wait_ms);

    set_all_leds(0);
    Serial.println("LED WIFI ON");
    RUN_TEST(test_led_wifi_on);
    delay(wait_ms);

    Serial.println("LED WIFI OFF");
    RUN_TEST(test_led_wifi_off);
    delay(wait_ms);

    set_all_leds(0);
    Serial.println("LED PCB HIGH");
    RUN_TEST(test_led_builtin_pcb_on);
    delay(wait_ms);

    Serial.println("LED PCB LOW");
    RUN_TEST(test_led_builtin_pcb_off);
    delay(wait_ms);

    UNITY_END(); // stop unit testing
}
