#include <Arduino.h>
#include <unity.h>

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

#include "gpio_pins.h"

void test_led_builtin_esp_high(void) {
    digitalWrite(LED_BUILTIN_ESP, HIGH);
    TEST_ASSERT_EQUAL(HIGH, digitalRead(LED_BUILTIN_ESP));
}

void test_led_builtin_esp_low(void) {
    digitalWrite(LED_BUILTIN_ESP, LOW);
    TEST_ASSERT_EQUAL(LOW, digitalRead(LED_BUILTIN_ESP));
}

void test_led_builtin_pcb_high(void) {
    digitalWrite(LED_BUILTIN_PCB, HIGH);
    TEST_ASSERT_EQUAL(HIGH, digitalRead(LED_BUILTIN_PCB));
}

void test_led_builtin_pcb_low(void) {
    digitalWrite(LED_BUILTIN_PCB, LOW);
    TEST_ASSERT_EQUAL(LOW, digitalRead(LED_BUILTIN_PCB));
}

void test_led_wifi_high(void) {
    digitalWrite(LED_WIFI, HIGH);
    TEST_ASSERT_EQUAL(HIGH, digitalRead(LED_WIFI));
}

void test_led_wifi_low(void) {
    digitalWrite(LED_WIFI, LOW);
    TEST_ASSERT_EQUAL(LOW, digitalRead(LED_WIFI));
}

void setup() {
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);

    UNITY_BEGIN();    // IMPORTANT LINE!
    //RUN_TEST(test_led_builtin_pin_number);

    pinMode(LED_BUILTIN_ESP, OUTPUT);
    pinMode(LED_BUILTIN_PCB, OUTPUT);
    pinMode(LED_WIFI, OUTPUT);
}

uint8_t i = 0;
uint8_t max_blinks = 5;

void loop() {
    if (i < max_blinks)
    {
        Serial.println("loop ");
        Serial.flush();
        RUN_TEST(test_led_builtin_esp_high);
        delay(500);
        RUN_TEST(test_led_builtin_esp_low);
        delay(500);
        RUN_TEST(test_led_builtin_pcb_high);
        delay(500);
        RUN_TEST(test_led_builtin_pcb_low);
        delay(500);
        RUN_TEST(test_led_wifi_high);
        delay(500);
        RUN_TEST(test_led_wifi_low);
        delay(500);
        i++;
    }
    else if (i == max_blinks) {
      UNITY_END(); // stop unit testing
    }
}
