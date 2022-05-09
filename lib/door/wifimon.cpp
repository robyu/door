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
        ENUM_DESCR_DECLARE(WM_REBOOT),
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
                                        int len_server,
                                        char *pmqtt_port,
                                        int len_port)
{
    File file;
    file = LittleFS.open(MQTT_FNAME, "r");
    if (!file)
    {
        memset(pmqtt_server, 0, len_server * sizeof(char));
        memset(pmqtt_port, 0, len_port * sizeof(char));
    }
    else
    {
        int numbytes;
        numbytes = file.readBytesUntil('\n', pmqtt_server, len_server);
        UTILS_ASSERT(numbytes < len_server);

        numbytes = file.readBytesUntil('\n', pmqtt_port, len_port);
        UTILS_ASSERT(numbytes < len_port);
    }
    return;
}

void wifimon_write_mqtt_params_to_file(char *pmqtt_server,
                                       int len_server,
                                       char *pmqtt_port,
                                       int len_port)
{
    File file;
    file = LittleFS.open(MQTT_FNAME, "w");
    
    if (!file)
    {
        Serial.println("Failed to open file " + String(MQTT_FNAME));
        return;
    }
    else
    {
        UTILS_ASSERT((int)strlen(pmqtt_server) < len_server);
        file.write(pmqtt_server, strlen(pmqtt_server)+1);

        UTILS_ASSERT((int)strlen(pmqtt_port) < len_port);
        file.write(pmqtt_port, strlen(pmqtt_port)+1);
    }
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
                                       WIFIMON_MAX_LEN_MQTT_SERVER,
                                       pstate->pmqtt_port,
                                       WIFIMON_MAX_LEN_MQTT_PORT);
    pstate->enable_restart = true;
}

/*
see state machine diagram
wifimon-state-machine.pdf
or
https://docs.google.com/drawings/d/1ZojjvD8IzcoGZNFNz5XOvnSn9KmD7tuCvef_ixaX7fY/edit?usp=sharing
*/

static wifimon_state_t do_update_logic(wifimon_t *pstate)
{
    wifimon_state_t next_state;
    int reset_button = switch_update_state(&pstate->reset_button);
    //Serial.println("do_update: initial state: " + state_to_string(pstate->curr_state));

    next_state = pstate->curr_state;
    switch(pstate->curr_state)
    {
    case WM_INIT:
        next_state = WM_CHECK_RECONFIG_BTN;
        break;

    case WM_CHECK_RECONFIG_BTN: // check if reset button pressed
        //Serial.println("WM_CHECK_RECONFIG_BTN_STATE");
        //Serial.println("  reset button = " + String(switch_update_state(&pstate->reset_button)));

        if (reset_button==0)
        {
            if (utils_get_elapsed_msec_and_reset(&pstate->start_time) > 2 * pstate->threshold_check_reset_ms)
            {

                // button is UP  AND we've waited a sufficiently long period,
                // so continue to next state
                //
                // we need to wait to give button time to settle; otherwise may not be detected
                next_state = WM_NOT_CONNECTED;
            }
        }
        else
        {
            // button is DOWN
            UTILS_ASSERT(1==reset_button);
            if (switch_get_state_duration_ms(&pstate->reset_button) >= pstate->threshold_check_reset_ms)
            {
                // long press -> go to reconfig
                // transition:  open config portal
                next_state = WM_RECONFIG;
            }
        }
        break;

    case WM_RECONFIG:
        // we don't expect to end up here, so something went wrong.
        // reboot.
        // transition:  reboot
        next_state = WM_REBOOT;
        break;

    case WM_NOT_CONNECTED:
        // see https://www.arduino.cc/en/Reference/WiFiStatus
        //Serial.println("WiFi.status = " + String(WiFi.status()));
        if (wifi_is_connected(pstate)==1)
        {
            next_state = WM_CONNECTED;
        }
        else if (utils_get_elapsed_msec_and_reset(&pstate->start_time) > pstate->threshold_not_connected_ms)
        {
            // we've been disconnected for too long
            next_state = WM_REBOOT;
        }
        else
        {
            Serial.printf("wifimon:do_update NOT_CONNECTED state for (%ld) msec\n", utils_get_elapsed_msec_and_reset(&pstate->start_time));
        }
        
            
        break;

    case WM_CONNECTED:
        //Serial.println("WiFi.status = " + String(WiFi.status()));
        if (wifi_is_connected(pstate)==0)
        {
            next_state = WM_NOT_CONNECTED;
        }
        break;

    case WM_REBOOT:
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

    //Serial.println("next_state: " + state_to_string(next_state));
    return next_state;
}

// see https://github.com/tzapu/WiFiManager/blob/master/examples/OnDemandConfigPortal/OnDemandConfigPortal.ino
static void start_config_portal(wifimon_t *pstate)
{
    WiFiManagerParameter custom_mqtt_server("server", "mqtt server", pstate->pmqtt_server, WIFIMON_MAX_LEN_MQTT_SERVER);
    WiFiManagerParameter custom_mqtt_port("port", "mqtt port", pstate->pmqtt_port, WIFIMON_MAX_LEN_MQTT_PORT);

    wifi_manager.addParameter(&custom_mqtt_server);
    wifi_manager.addParameter(&custom_mqtt_port);

    //sets timeout until configuration portal gets turned off
    //useful to make it all retry or go to sleep
    //in seconds
    wifi_manager.setConfigPortalTimeout(pstate->threshold_reconfig_sec);

    Serial.println("start_config_portal");
    bool is_connected = wifi_manager.startConfigPortal("door-config");
    if (is_connected==false) {
        Serial.println("  failed to connect and hit timeout");
    }

    // save mqtt data
    strncpy(pstate->pmqtt_server, custom_mqtt_server.getValue(), WIFIMON_MAX_LEN_MQTT_SERVER);
    strncpy(pstate->pmqtt_port, custom_mqtt_port.getValue(), WIFIMON_MAX_LEN_MQTT_PORT);
    Serial.println("\tmqtt_server : " + String(pstate->pmqtt_server));
    Serial.println("\tmqtt_port : " + String(pstate->pmqtt_port));
    wifimon_write_mqtt_params_to_file(pstate->pmqtt_server,
                                      WIFIMON_MAX_LEN_MQTT_SERVER,
                                      pstate->pmqtt_port,
                                      WIFIMON_MAX_LEN_MQTT_PORT);
    
    if (pstate->enable_restart)
    {
        Serial.println("enable_restart=true...restarting");
        
        utils_restart();
    }
}

static void do_transitions(wifimon_t *pstate, wifimon_state_t next_state)
{
    if (next_state==pstate->curr_state)
    {
        ; // do nothing
    }
    else
    {
        Serial.printf("do_transitions: %s -> %s\n", state_to_string(pstate->curr_state), state_to_string(next_state));
        switch(next_state)
        {
        case WM_CHECK_RECONFIG_BTN:
            pstate->start_time = millis();  // elapsed time button press
            break;
            
        case WM_RECONFIG:
            break;

        case WM_NOT_CONNECTED:
            pstate->start_time = millis(); // elapsed time disconnected
            break;

        case WM_CONNECTED:
            pstate->led_counter = 1;
            //tx_send("connected_to_wifi","");
            break;

        case WM_REBOOT:
            break;

        default:
            // shouldn't ever reach this point
            Serial.printf("wifimon::do_transitions illegal state %s\n", state_to_string(pstate->curr_state));
            UTILS_ASSERT(0);
            
        }
        pstate->curr_state = next_state;
    }
}

static void do_steadystate(wifimon_t *pstate)
{
    switch(pstate->curr_state)
    {
    case WM_INIT:
        break;

    case WM_CHECK_RECONFIG_BTN:
        pstate->led_counter++;
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
        wifi_manager.autoConnect();
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

    case WM_REBOOT:
        utils_set_led(pstate->led_pin, 0); // blank LED
        if (pstate->enable_restart)
        {
            utils_restart();
        }
        break;
    default:
        UTILS_ASSERT(0);
    }
}

wifimon_state_t wifimon_update(wifimon_t *pstate)
{
    wifimon_state_t next_state;
    next_state = do_update_logic(pstate);

    do_transitions(pstate, next_state);
    do_steadystate(pstate);

    return pstate->curr_state;
}

void wifimon_force_transition(wifimon_t *pstate, wifimon_state_t next_state)
{
    pstate->debug_next_state = next_state;
}
