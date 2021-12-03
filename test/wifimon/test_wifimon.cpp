#include <Arduino.h>
#include <unity.h>
#include "LittleFS.h"
#include "wifimon.h"
#include "gpio_pins.h"
#include <DNSServer.h> 
#include <ESP8266WebServer.h> 
#include <ESP8266WiFi.h> 
#include <WiFiManager.h>

wifimon_t wifimon_state;

void setup() {
    Serial.begin(115200);
    delay(1000); // pause for a bit

    UNITY_BEGIN();    // IMPORTANT LINE!
}

void test_force_reconfig(void)
{
    wifimon_init(&wifimon_state,
                 LED_WIFI,
                 BTN_RESET);
    wifimon_force_transition(&wifimon_state,
                             WM_RECONFIG);
    wifimon_state.enable_restart=false;  // dont restart during testing
    wifimon_state.threshold_reconfig_sec = 30;
    
    Serial.println("==================================");
    Serial.println("Forcing wifimon into reconfig mode");
    Serial.println("1. VERIFY door-config access point available");
    Serial.println("2. VERIFY AP form has all fields");
    Serial.println("3. VERIFY that wifi LED is on");

    wifimon_update(&wifimon_state);

    //
    // if we got this far, we passed
    TEST_ASSERT_TRUE(true);
}

void wifiInfo(WiFiManager *wm){
  WiFi.printDiag(Serial);
  Serial.println("SAVED: " + (String)wm->getWiFiIsSaved() ? "YES" : "NO");
  Serial.println("SSID: " + (String)wm->getWiFiSSID());
  Serial.println("PASS: " + (String)wm->getWiFiPass());
  Serial.println("Connected: " + String(WiFi.status()==WL_CONNECTED));
}

void test_open_ap_portal_then_quit(void)
{
    WiFiManager wm;
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP IS THIS NECESSARY?
    wm.debugPlatformInfo();
    wm.setConfigPortalTimeout(5); // return from config portal after N seconds, no matter what
    wifiInfo(&wm);
    wm.startConfigPortal("door-config");

    // if we get this far, the test passed
    TEST_ASSERT_TRUE(true);
}

    /*
      reconfig: solid
      check reset: fast blink
      not connected: off
      connected: slow blink
    */
void test_led_check_reset(void)
{
    wifimon_t wifimon;
    long unsigned time0_ms;
    long unsigned  duration_ms = 5000;

    wifimon_init(&wifimon, 
                 LED_WIFI,
                 BTN_RESET);
    wifimon.enable_restart = false;
    wifimon.pretend_network_connected = false;

    // enter check_reset state 
    Serial.println("==========================");
    Serial.println("forcing wifimon into CHECK_RESET for " + String(duration_ms) + " ms");
    Serial.println("VERIFY LED state==fast blink");
    
    time0_ms = millis();
    while(millis() - time0_ms < duration_ms)
    {
        wifimon_force_transition(&wifimon, WM_CHECK_RESET);
        wifimon_update(&wifimon);
        delay(100);
    }
    TEST_ASSERT_TRUE(true);
}

void test_led_connected(void)
{
    wifimon_t wifimon;
    long unsigned time0_ms;
    long unsigned  duration_ms = 5000;

    wifimon_init(&wifimon, 
                 LED_WIFI,
                 BTN_RESET);
    wifimon.enable_restart = false;
    wifimon.pretend_network_connected = false;

    // enter check_reset state 
    Serial.println("==========================");
    Serial.println("forcing wifimon into CONNECTED for " + String(duration_ms) + " ms");
    Serial.println("VERIFY LED state==slow blink");
    
    time0_ms = millis();
    while(millis() - time0_ms < duration_ms)
    {
        wifimon_force_transition(&wifimon, WM_CONNECTED);
        wifimon_update(&wifimon);
        delay(100);
    }
    TEST_ASSERT_TRUE(true);
}

void loop() {
    // RUN_TEST(test_open_ap_portal_then_quit);
    RUN_TEST(test_force_reconfig);
    // RUN_TEST(test_led_check_reset);
    // RUN_TEST(test_led_connected);

// STOPPED HERE
// run from first state to connected
// verify mqtt parameters    

// delete all stored parameters
// then open config ap
//  then verify parameters    
    
    
    UNITY_END();
}
