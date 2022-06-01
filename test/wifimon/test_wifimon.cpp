#include <Arduino.h>
#include <unity.h>
#include "LittleFS.h"
#include "wifimon.h"
#include "gpio_pins.h"
#include <DNSServer.h> 
#include <ESP8266WebServer.h> 
#include <ESP8266WiFi.h> 
#include <WiFiManager.h>
#include "utils.h"
#include "json_config.h"

wifimon_t wifimon_state;

#define JSONFNAME "/unit_tests.json"

void get_wifi_credential(const char *fname, char *ssid_ptr, size_t len_ssid, char *pwd_ptr, size_t len_pwd)
{
    DynamicJsonDocument json(1024);
    jc_get_config_from_file(fname, &json);
    
    Serial.printf("ssid=(%s)\n",
                  (const char *)json["wifi_access"]["ssid"]);
    Serial.printf("pwd=(%s)\n",
                  (const char *)json["wifi_access"]["pwd"]);

    TEST_ASSERT_TRUE(strlen((const char *)json["wifi_access"]["ssid"]) < len_ssid);
    strncpy(ssid_ptr,
            json["wifi_access"]["ssid"].as<const char *>(),
            len_ssid);

    TEST_ASSERT_TRUE(strlen((const char *)json["wifi_access"]["pwd"]) < len_pwd);
    strncpy(pwd_ptr,
            json["wifi_access"]["pwd"].as<const char *>(),
            len_pwd);
}

void setup() {
    Serial.begin(115200);
    delay(1000); // pause for a bit

    UNITY_BEGIN();    // IMPORTANT LINE!
}

void test_init_and_quit(void)
{
    WiFiManager wm;
    Serial.printf("TEST_INIT_AND_QUIT==================================\n");
    Serial.printf("wifimon init\n");
    wifimon_init(&wifimon_state,
                 LED_WIFI,
                 BTN_RESET);
    wifimon_state.threshold_reconfig_sec = 30;
    
    Serial.printf("wifimon update\n");
    wifimon_update(&wifimon_state);

    Serial.printf("exit\n");
    TEST_ASSERT_TRUE(wm.getWiFiIsSaved()==1);
}

void test_force_reconfig(void)
{
    WiFiManager wm;
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

    TEST_ASSERT_TRUE(wm.getWiFiIsSaved()==1);
}

/*
void wifiInfo(WiFiManager *wm){
    Serial.println("====================================");
    WiFi.printDiag(Serial);
    Serial.println("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=");
    Serial.println("SAVED: " + String(wm->getWiFiIsSaved() ));
    Serial.println("SSID: " + String(wm->getWiFiSSID()) );
    Serial.println("PASS: " + String(wm->getWiFiPass()) );
    Serial.println("Connected: " + String(WiFi.status()==WL_CONNECTED));
    Serial.println("====================================");
}
*/

void test_open_ap_portal_then_quit(void)
{
    WiFiManager wm;
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP IS THIS NECESSARY?
    wm.debugPlatformInfo();
    wm.setConfigPortalTimeout(5); // return from config portal after N seconds, no matter what
    wifimon_print_info(&wm, NULL);
    wm.startConfigPortal("door-config");

    wifimon_print_info(&wm, NULL);

    TEST_ASSERT_TRUE(wm.getWiFiIsSaved()==1);
}

    /*
      reconfig: solid
      check reset: fast blink
      not connected: off
      connected: slow blink
    */
void test_led_check_reconfig(void)
{
    WiFiManager wm;
    wifimon_t wifimon;
    long unsigned time0_ms;
    long unsigned  duration_ms = 5000;

    wifimon_init(&wifimon, 
                 LED_WIFI,
                 BTN_RESET);
    wifimon.enable_restart = false;
    wifimon.pretend_network_connected = false;

    // enter check_reconfig state 
    Serial.println("==========================");
    Serial.println("forcing wifimon into CHECK_RECONFIG for " + String(duration_ms) + " ms");
    Serial.println("VERIFY LED state==fast blink");
    
    time0_ms = millis();
    while(millis() - time0_ms < duration_ms)
    {
        wifimon_force_transition(&wifimon, WM_CHECK_RECONFIG_BTN);
        wifimon_update(&wifimon);
        delay(100);
    }
    TEST_ASSERT_TRUE(wm.getWiFiIsSaved()==1);
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

    // enter check_reconfig state 
    Serial.println("");
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

void test_wifi_param_file_read_write(void)
{
    wifimon_t wifimon;
    char pmqtt_server[128];
    short mqtt_port;

    char pmqtt_server2[WIFIMON_MAX_LEN_MQTT_SERVER];
    short mqtt_port2;

    wifimon_init(&wifimon, 
                 LED_WIFI,
                 BTN_RESET);
    wifimon.enable_restart = false;
    wifimon.pretend_network_connected = false;


    /*
      generate random params
    */
    {
        int n0,n1,n2,n3;
        
        UTILS_ZERO_STRUCT(pmqtt_server);

        n0 = random(255);
        n1 = random(255);
        n2 = random(255);
        n3 = random(255);
        sprintf(pmqtt_server, "%d.%d.%d.%d",n0,n1,n2,n3);

        mqtt_port = (short)random(1024);
    }
    
    Serial.printf("mqtt_server = (%s)\n",pmqtt_server);
    Serial.printf("mqtt_port = (%d)\n",mqtt_port);
    wifimon_write_mqtt_params_to_file(pmqtt_server,
                                      mqtt_port);

    wifimon_read_mqtt_params_from_file(pmqtt_server2,
                                       &mqtt_port2);
    Serial.printf("mqtt_server2 = (%s)\n",pmqtt_server2);
    Serial.printf("mqtt_port2 = (%d)\n",mqtt_port2);

    TEST_ASSERT_TRUE(String(pmqtt_server)==String(pmqtt_server2));
    TEST_ASSERT_TRUE(mqtt_port==mqtt_port2);
    
}

void test_loop(void)
{
    WiFiManager wm;
    int n;
    Serial.printf("TEST_LOOP ==================================\n");
    Serial.printf("wifimon init\n");
    wifimon_init(&wifimon_state,
                 LED_WIFI,
                 BTN_RESET);
    wifimon_state.threshold_reconfig_sec = 30;
    
    n = 0;
    while((n < 30) && (wifimon_state.curr_state != WM_CONNECTED))
    {
        Serial.printf("\n");
        Serial.printf("\niteration %d\n",n);
        wifimon_update(&wifimon_state);
        delay(1000);
        n++;
    }

    Serial.printf("exit\n");
    TEST_ASSERT_TRUE(wifimon_state.curr_state == WM_CONNECTED);
}

void test_wifi_begin(void)
{
    WiFiManager wm;
    char pssid[255];
    char ppwd[255];
    int n;
    
    get_wifi_credential(JSONFNAME, pssid, 255, ppwd, 255);

    WiFi.mode(WIFI_STA);  
    WiFi.begin(pssid, ppwd);
    delay(1000);
    for (n=0;n<20;n++)
    {
        Serial.printf("[%d] connections status = %d\n", n, WiFi.status()==WL_CONNECTED);
        delay(500);
    }
    TEST_ASSERT_TRUE(WiFi.status()==WL_CONNECTED);
    
}

void loop() {
    // RUN_TEST(test_init_and_quit);
    // RUN_TEST(test_open_ap_portal_then_quit);
    // RUN_TEST(test_force_reconfig);
    // RUN_TEST(test_led_check_reconfig);
    // RUN_TEST(test_led_connected);
    // RUN_TEST(test_wifi_param_file_read_write);

    RUN_TEST(test_loop);
    //RUN_TEST(test_wifi_begin);
    UNITY_END();
}
