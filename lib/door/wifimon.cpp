#include "Arduino.h"
#include "WiFiClient.h"
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <assert.h>
#include "wifimon.h"
#include "switch.h"
#include "utils.h"
#include "enum_descr.h"
#include "LittleFS.h"

/*
BLINK PATTERNS
====================
no blink = WM_NOT_CONNECTED OR REBOOT
fast blink = WM_CHECK_RECONFIG_BTN
slow blink = WM_CONNECTED
solid = WM_RECONFIG (AP mode)

blink intervals assume delay(100) in main loop
*/
#define LED_SLOW_BLINK 10
#define LED_FAST_BLINK  1

#define PORTAL_TIMEOUT_SEC 90

WiFiManager wifi_manager;

/*
make a little mapping of enums-to-strings,
so you can print the string version of an enum
*/
static const char* state_to_string(wifimon_state_t state)
{
    enum_descr_t wifimon_state_descr[] = {
        ENUM_DESCR_DECLARE(WM_DONT_USE),
        ENUM_DESCR_DECLARE(WM_INIT),
        ENUM_DESCR_DECLARE(WM_CHECK_RECONFIG_BTN),
        ENUM_DESCR_DECLARE(WM_RECONFIG),
        ENUM_DESCR_DECLARE(WM_NOT_CONNECTED),
        ENUM_DESCR_DECLARE(WM_CONNECTED),
        ENUM_DESCR_DECLARE(WM_LAST_DONT_USE),
        ENUM_DESCR_END
    };

    return wifimon_state_descr[(int)state].pvalue_str;
}

static int wifi_is_connected(const wifimon_t *pstate)
{
    wl_status_t status;
    if (pstate->pretend_network_connected)
    {
        status = WL_CONNECTED;
    }
    else
    {
        status = WiFi.status();
    }
    return (status==WL_CONNECTED);
}

#define MQTT_FNAME "/mqtt_params.txt"

void wifimon_read_mqtt_params_from_file(char *pmqtt_server,
                                        int *pmqtt_port)

{
    File file;
    file = LittleFS.open(MQTT_FNAME, "r");
    if (!file)
    {
        memset(pmqtt_server, 0, WIFIMON_MAX_LEN_MQTT_SERVER);
        *pmqtt_port = 0;
    }
    else
    {
        file.readBytes(pmqtt_server, WIFIMON_MAX_LEN_MQTT_SERVER);
        file.readBytes((char *)pmqtt_port, sizeof(*pmqtt_port));
    }

    Serial.printf("wifimon_read_mqtt_params_from_file:\n");
    Serial.printf("  mqtt server: %s\n", pmqtt_server);
    Serial.printf("  mqtt port: %d\n", *pmqtt_port);

    
    return;
}

void wifimon_print_info(WiFiManager *wm, wifimon_t *pstate)
{
    Serial.printf("  WIFIMON INFO PARAMS=====================\n");
    Serial.printf("  SAVED: %d\n",  wm->getWiFiIsSaved() );
    Serial.printf("  SSID: %s\n", wm->getWiFiSSID().c_str());
    Serial.printf("  PASS: %s\n", wm->getWiFiPass().c_str());;
    Serial.printf("  Connected: %d\n", WiFi.status()==WL_CONNECTED);
    Serial.printf("  -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
    WiFi.printDiag(Serial);
    Serial.printf("  -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
    Serial.printf("\n");
    if (pstate)
    {
        Serial.printf("  mqtt server: %s\n", pstate->pmqtt_server);
        Serial.printf("  mqtt port: %d\n", pstate->mqtt_port);
    }
    Serial.printf("====================================\n");
}

void wifimon_write_mqtt_params_to_file(const char *pmqtt_server,
                                       int mqtt_port)
{
    File file;
    file = LittleFS.open(MQTT_FNAME, "w");
    
    if (!file)
    {
        Serial.println("Failed to open file " + String(MQTT_FNAME));
        return;
    }

    UTILS_ASSERT((int)strlen(pmqtt_server) < WIFIMON_MAX_LEN_MQTT_SERVER);
    file.write(pmqtt_server, WIFIMON_MAX_LEN_MQTT_SERVER);
    
    file.write((uint8_t *)&mqtt_port, sizeof(mqtt_port));
    Serial.printf("wrote mqtt params to %s\n", MQTT_FNAME);
    
    return;
}

void wifimon_init(wifimon_t *pstate, int led_pin, int reconfig_button_pin)
{
    UTILS_ZERO_STRUCT(pstate);

    pstate->led_pin = led_pin;

    if (led_pin >= 0)
    {
        pinMode(led_pin, OUTPUT);
    }
    switch_init(&pstate->reset_button, reconfig_button_pin);

    // Station mode == connect to previously saved access point
    // Access Point Mode == for configuration, act as access point
    //
    // force station mode because if device was switched off while in access point mode,
    // it will startup in access point mode again
    // see https://github.com/kentaylor/WiFiManager/blob/master/examples/ConfigOnSwitch/ConfigOnSwitch.ino#L46
    pstate->curr_state = WM_INIT;
    pstate->threshold_check_reset_ms = 1000;
    pstate->threshold_reconfig_button_ms = 5000;
    pstate->threshold_reconfig_sec = 180; 
    pstate->threshold_not_connected_ms = 20000;

    // try to load mqtt parameters from filesystem
    bool b = LittleFS.begin();
    if(!b)
    {
        Serial.println("An Error has occurred while mounting LittleFS");
        UTILS_ASSERT(0);
    }

        
    wifimon_read_mqtt_params_from_file(pstate->pmqtt_server,
                                       &pstate->mqtt_port);
    pstate->enable_restart = true;
}

/*
see state machine diagram
wifimon-state-machine.pdf
or
https://docs.google.com/drawings/d/1ZojjvD8IzcoGZNFNz5XOvnSn9KmD7tuCvef_ixaX7fY/edit?usp=sharing
*/

static wifimon_state_t do_transitions(wifimon_t *pstate)
{
    wifimon_state_t next_state;
    int reset_button = switch_update_state(&pstate->reset_button);
    //Serial.println("do_update: initial state: " + state_to_string(pstate->curr_state));

    next_state = pstate->curr_state;
    switch(pstate->curr_state)
    {
    case WM_INIT:
        next_state = WM_CHECK_RECONFIG_BTN;
        pstate->start_time = millis();
        pstate->reconfig_loop_cnt = 0;
        break;

    case WM_CHECK_RECONFIG_BTN: // check if reset button pressed
        //Serial.println("WM_CHECK_RECONFIG_BTN_STATE");
        Serial.println("  (WM_CHECK_RECONFIG_BTN): reset button = " + String(switch_update_state(&pstate->reset_button)));

        if (reset_button==0)
        {
            if ((utils_get_elapsed_msec_and_reset(&pstate->start_time) > 2 * pstate->threshold_check_reset_ms) &&
                (pstate->reconfig_loop_cnt >= 5))
            {

                // button is UP  AND we've waited a sufficiently long period,
                // so continue to next state
                //
                // we need to wait to give button time to settle; otherwise may not be detected
                WiFi.mode(WIFI_STA);  
                next_state = WM_NOT_CONNECTED;

                pstate->start_time = millis(); // elapsed time disconnected
                Serial.printf("wifimon: calling WiFi.begin()\n");
                wifimon_print_info(&wifi_manager, pstate);
                WiFi.begin();
                Serial.printf("wifimon: returned from WiFi.begin()\n");
            }
        }
        else
        {
            // button is DOWN
            UTILS_ASSERT(1==reset_button);

            // restart loop count
            pstate->reconfig_loop_cnt = 0;
            
            if (switch_get_state_duration_ms(&pstate->reset_button) >= pstate->threshold_check_reset_ms)
            {
                // long press -> go to reconfig
                // transition:  open config portal
                next_state = WM_RECONFIG;
            }
        }
        break;

    case WM_RECONFIG:
        // should never reach this state
        UTILS_ASSERT(0);
        break;

    case WM_NOT_CONNECTED:
    {
        long elapsed_ms = utils_get_elapsed_msec_and_reset(&pstate->start_time);
        // see https://www.arduino.cc/en/Reference/WiFiStatus
        //Serial.println("WiFi.status = " + String(WiFi.status()));

        //
        // assume that wifi will eventually connect
        // (usually takes 8-10 sec)
        // leave it up to higher level state machine to reboot if necessary
        
        Serial.printf("WM_NOT_CONNECTED: elapsed time = (%f) sec\n",elapsed_ms/1000.0);
        if (wifi_is_connected(pstate)==1)
        {
            IPAddress ipaddr = WiFi.localIP();
            Serial.printf("WM_CONNECTED: got IP address (%s)\n", ipaddr.toString().c_str());
            next_state = WM_CONNECTED;
        }
        break;
    }

    case WM_CONNECTED:
        //Serial.println("WiFi.status = " + String(WiFi.status()));
        if (wifi_is_connected(pstate)==0)
        {
            Serial.printf("WM_CONNECTED: lost WiFi connection!\n");
            next_state = WM_NOT_CONNECTED;
        }
        break;

    default:
        Serial.println("wifimon::do_update illegal state " + String(state_to_string(pstate->curr_state)));
        UTILS_ASSERT(0);
    }

    //
    // force transition?
    if (pstate->debug_next_state != WM_DONT_USE)
    {
        next_state = pstate->debug_next_state;
        pstate->debug_next_state = WM_DONT_USE;
    }

    if (pstate->curr_state != next_state)
    {
        Serial.printf("TRANSITION (%s) -> (%s)\n", state_to_string(pstate->curr_state),  state_to_string(next_state));
    }

    return next_state;
}

// see https://github.com/tzapu/WiFiManager/blob/master/examples/OnDemandConfigPortal/OnDemandConfigPortal.ino
static void start_config_portal(wifimon_t *pstate)
{
    char pmqtt_port_char[WIFIMON_MAX_LEN_MQTT_SERVER];
    
    UTILS_ZERO_STRUCT(pmqtt_port_char);
    strncpy(pmqtt_port_char, String(pstate->mqtt_port).c_str(), WIFIMON_MAX_LEN_MQTT_SERVER);

    // see WiFiManager.cpp for ctor definitions
    // WiFiManagerParameter::WiFiManagerParameter(const char *id, const char *label, const char *defaultValue, int length) {
    WiFiManagerParameter custom_mqtt_server("server", "mqtt server", pstate->pmqtt_server, WIFIMON_MAX_LEN_MQTT_SERVER);
    WiFiManagerParameter custom_mqtt_port("port", "mqtt port", pmqtt_port_char, WIFIMON_MAX_LEN_MQTT_SERVER);

    wifi_manager.addParameter(&custom_mqtt_server);
    wifi_manager.addParameter(&custom_mqtt_port);

    //sets timeout until configuration portal gets turned off
    //useful to make it all retry or go to sleep
    //in seconds
    //wifi_manager.setConfigPortalTimeout(pstate->threshold_reconfig_sec);
    wifi_manager.setConfigPortalTimeout(PORTAL_TIMEOUT_SEC);

    Serial.println("start_config_portal");
    bool is_connected = wifi_manager.startConfigPortal("door-config");
    if (is_connected==false) {
        Serial.println("  failed to connect and hit timeout");
    }

    // save mqtt data
    strncpy(pstate->pmqtt_server, custom_mqtt_server.getValue(), WIFIMON_MAX_LEN_MQTT_SERVER);

    Serial.printf("==============================================\n");
    Serial.printf("custom_mqtt_port.getValue() = (%s)\n", custom_mqtt_port.getValue());
    Serial.printf("pmqtt_port_char = (%s)\n", pmqtt_port_char);
    Serial.printf("==============================================\n");

    
    //short tmp_port = (short)atoi(custom_mqtt_port.getValue());
    int tmp_port = (unsigned short)atoi(custom_mqtt_port.getValue());
    if ((tmp_port < 0) || (tmp_port > 65535))
    {
        tmp_port = 0;
    }
    Serial.printf("tmp_port = (%d)\n",tmp_port);
    pstate->mqtt_port = tmp_port;
    Serial.printf("mqtt_server : %s\n", pstate->pmqtt_server);
    Serial.printf("mqtt_port : %d\n", pstate->mqtt_port);
    wifimon_write_mqtt_params_to_file(pstate->pmqtt_server,
                                      pstate->mqtt_port);
    
    if (pstate->enable_restart)
    {
        Serial.println("enable_restart=true...restarting");
        
        utils_restart();
    }
}

static void do_steadystate(wifimon_t *pstate)
{
    Serial.printf("wifimon STEADY STATE (%s)\n", state_to_string(pstate->curr_state));
    switch(pstate->curr_state)
    {
    case WM_INIT:
        break;

    case WM_CHECK_RECONFIG_BTN:
        pstate->led_counter++;
        pstate->reconfig_loop_cnt++;
        if ((pstate->led_counter % LED_FAST_BLINK)==0)
        {
            pstate->led_state ^= 1;
            utils_set_led(pstate->led_pin, pstate->led_state); // fast blink
        }
        break;

    case WM_RECONFIG:
        utils_set_led(pstate->led_pin, 1); // solid LED
        start_config_portal(pstate);

        // this state never returns (it reboots)
        break;

    case WM_NOT_CONNECTED:
        utils_set_led(pstate->led_pin, 0); // dead LED
        break;
      
    case WM_CONNECTED:
        pstate->led_counter++;
        if ((pstate->led_counter % LED_SLOW_BLINK)==0)
        {
            pstate->led_state ^= 1;
            //Serial.printf("led_counter=%d led_state=%d\n",pstate->led_counter,pstate->led_state);
            utils_set_led(pstate->led_pin, pstate->led_state); // slow blink
        }
        break;

    default:
        UTILS_ASSERT(0);
    }
}

wifimon_state_t wifimon_update(wifimon_t *pstate)
{
    do_steadystate(pstate);
    pstate->curr_state = do_transitions(pstate);

    return pstate->curr_state;
}

void wifimon_force_transition(wifimon_t *pstate, wifimon_state_t next_state)
{
    pstate->debug_next_state = next_state;
}
