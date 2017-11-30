#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include "WiFiClient.h"
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include "utils.h"
#include <assert.h>

#define WIFI_MAX_LEN_STRING 255
typedef struct
{
    const char *pssid;
    const char *ppwd;
    int led_pin;
} wifi_t;


/*
  if already connected to wifi, do nothing
  if not connected, then connect to wifi
*/
void wifi_connect(wifi_t *pstate)
{
    int led_state = 0;

    Serial.println("trying to connect");
    WiFi.mode(WIFI_STA);
    WiFi.begin(pstate->pssid, pstate->ppwd);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
        utils_set_led(pstate->led_pin, 0);
        led_state ^= 1;
    }

    Serial.println("");
    Serial.println("WiFi connected");  
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    utils_set_led(pstate->led_pin, 1);
    return;
}


/*
  pssid = ssid to connect to
  ppwd = password
  led_pin = pin # with indicator LED; specify -1 to bypass LED
*/
void wifi_init(wifi_t *pstate,
               const char *pssid,
               const char *ppwd,
               int led_pin)
{
    memset(pstate, 0, sizeof(*pstate));
    // https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/examples/WiFiClient/WiFiClient.ino
    assert(strlen(pssid) < WIFI_MAX_LEN_STRING);
    assert(strlen(ppwd) < WIFI_MAX_LEN_STRING);
    pstate->pssid = pssid;
    pstate->ppwd = ppwd;
    
    if (led_pin >= 0)
    {
        pstate->led_pin = led_pin;
        pinMode(led_pin, OUTPUT);
    }
    else
    {
        pstate->led_pin = -1;
    }

    // try to connect during init
    wifi_connect(pstate);
}

void wifi_report_status(wifi_t *pstate)
{
    if  (WiFi.status() != WL_CONNECTED) 
    {
        Serial.print("not connected");
        utils_set_led(pstate->led_pin, 0);
    }
    else
    {
        Serial.println("WiFi connected");  
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        utils_set_led(pstate->led_pin, 1);
    }
}
