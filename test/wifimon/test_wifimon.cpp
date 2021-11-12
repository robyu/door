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
    wifimon_state.restart_after_config=false;
    wifimon_state.threshold_reconfig_sec = 30;
    
    Serial.println("==================================");
    Serial.println("Forcing wifimon into reconfig mode");
    Serial.println("1. verify door-config access point available");
    Serial.println("2. verify AP form");
    Serial.println("3. verify that wifi LED is on");

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
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
    wm.debugPlatformInfo();
    wm.setConfigPortalTimeout(5); // return from config portal after N seconds, no matter what
    wifiInfo(&wm);
    wm.startConfigPortal("door-config");

    // if we get this far, the test passed
    TEST_ASSERT_TRUE(true);
}


void loop() {
    RUN_TEST(test_open_ap_portal_then_quit);
    RUN_TEST(test_force_reconfig);

    UNITY_END();
}
