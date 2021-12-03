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

#define LED_SLOW_BLINK 10
#define LED_FAST_BLINK  2   

static enum_descr_t wifimon_state_descr[] =
{
    ENUM_DESCR_DECLARE(WM_DONT_USE),
    ENUM_DESCR_DECLARE(WM_INIT),
    ENUM_DESCR_DECLARE(WM_CHECK_RESET),
    ENUM_DESCR_DECLARE(WM_RECONFIG),
    ENUM_DESCR_DECLARE(WM_NOT_CONNECTED),
    ENUM_DESCR_DECLARE(WM_CONNECTED),
    ENUM_DESCR_DECLARE(WM_REBOOT),
    ENUM_DESCR_DECLARE(WM_LAST_DONT_USE),
    ENUM_DESCR_END
};

static String state_to_string(wifi_state_t state)
{
    return String(wifimon_state_descr[(int)state].pvalue_str);
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

void wifimon_init(wifimon_t *pstate, int led_pin, int reset_button_pin)
{
    //memset(pstate, 0, sizeof(*pstate));
    pstate->led_pin = led_pin;

    if (led_pin >= 0)
    {
        pinMode(led_pin, OUTPUT);
    }
    switch_init(&pstate->reset_button, reset_button_pin);

    pstate->curr_state = WM_INIT;
    pstate->threshold_check_reset_ms = 1000;
    pstate->threshold_reboot_button_ms = 5000;
    pstate->threshold_reconfig_sec = 180; 
    pstate->threshold_not_connected_ms = 20000;

    // try to load mqtt parameters from filesystem
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

static wifi_state_t do_update(wifimon_t *pstate)
{
    wifi_state_t next_state;

    Serial.println("do_update: initial state: " + state_to_string(pstate->curr_state));

    next_state = pstate->curr_state;
    switch(pstate->curr_state)
    {
    case WM_INIT:
        next_state = WM_CHECK_RESET;
        break;

    case WM_CHECK_RESET:
        Serial.println("WM_CHECK_RESET_STATE");
        Serial.println("  reset button = " + String(switch_update_state(&pstate->reset_button)));

        if ((switch_update_state(&pstate->reset_button)==0) &&
            (utils_get_elapsed_msec_and_reset(&pstate->start_time) > 2 * pstate->threshold_check_reset_ms)) 
        {
            // button is UP  AND we've waited a sufficiently long period,
            // so continue to next state
            //
            // we need to wait to give button time to settle; otherwise may not be detected
            next_state = WM_NOT_CONNECTED;
        }
        else
        {
            // button is DOWN
            if (switch_get_state_duration(&pstate->reset_button) >= pstate->threshold_check_reset_ms)
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
        Serial.println("WiFi.status = " + String(WiFi.status()));
        if (wifi_is_connected(pstate)==1)
        {
            // transition: light up indicator LED
            next_state = WM_CONNECTED;
        }
        else
        {
            long duration = utils_get_elapsed_msec_and_reset(&pstate->start_time);
            if (duration > pstate->threshold_not_connected_ms)
            {
                // failed to connect wifi in time, so reboot
                next_state = WM_REBOOT;
            }
            if ((switch_update_state(&pstate->reset_button)==1) &&
                (switch_get_state_duration(&pstate->reset_button) >= pstate->threshold_reboot_button_ms))
            {
                // reset button held down, so reboot
                Serial.println("RESET button held down: reboot");
                next_state = WM_REBOOT;
                
            }
        }
        break;

    case WM_CONNECTED:
        Serial.println("WiFi.status = " + String(WiFi.status()));
        if (wifi_is_connected(pstate)==0)
        {
            next_state = WM_NOT_CONNECTED;
        }
        if ((switch_update_state(&pstate->reset_button)==1) &&
            (switch_get_state_duration(&pstate->reset_button) >= pstate->threshold_reboot_button_ms))
        {
            // reset button held down, so reboot
            Serial.println("RESET button held down: reboot");
            next_state = WM_REBOOT;
            
        }
        break;

    case WM_REBOOT:
        break;
        
    default:
        Serial.println("wifimon::do_update illegal state " + state_to_string(pstate->curr_state));
        UTILS_ASSERT(0);
    }

    //
    // force transition?
    if (pstate->debug_next_state != WM_DONT_USE)
    {
        Serial.println("wifimon: forcing next state = " + state_to_string(pstate->debug_next_state));
        next_state = pstate->debug_next_state;
        pstate->debug_next_state = WM_DONT_USE;
    }
    Serial.println("next_state: " + state_to_string(next_state));
    return next_state;
}

// see https://github.com/tzapu/WiFiManager/blob/master/examples/OnDemandConfigPortal/OnDemandConfigPortal.ino
static void start_config_portal(wifimon_t *pstate)
{
    WiFiManager wifi_manager;
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

static void do_transitions(wifimon_t *pstate, wifi_state_t next_state)
{
    if (next_state==pstate->curr_state)
    {
        ; // do nothing
    }
    else
    {
        Serial.println("do_transitions: " + state_to_string(pstate->curr_state) + "->" + state_to_string(next_state));
        switch(next_state)
        {
        case WM_INIT:
            UTILS_ASSERT(0);
            break;
            
        case WM_CHECK_RESET:
            pstate->start_time = millis();
            break;
            
        case WM_RECONFIG:
            utils_set_led(pstate->led_pin, 1);
            start_config_portal(pstate);
            break;

        case WM_NOT_CONNECTED:
            pstate->start_time = millis();
            utils_set_led(pstate->led_pin, 0);
            break;

        case WM_CONNECTED:
            pstate->led_counter = 1;
            //tx_send("connected_to_wifi","");
            break;

        case WM_REBOOT:
            utils_set_led(pstate->led_pin, 0);
            if (pstate->enable_restart)
            {
                utils_restart();
            }
            break;

        default:
            // shouldn't ever reach this point
            Serial.println("wifimon::do_transitions illegal state " + state_to_string(pstate->curr_state));
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

    case WM_CHECK_RESET:
        pstate->led_counter++;
        utils_set_led(pstate->led_pin, pstate->led_counter % LED_FAST_BLINK);
        break;

    case WM_RECONFIG:
        break;

    case WM_NOT_CONNECTED:
        break;
      
    case WM_CONNECTED:
        pstate->led_counter++;
        utils_set_led(pstate->led_pin, pstate->led_counter % LED_SLOW_BLINK);
        break;

    case WM_REBOOT:
        break;
    default:
        UTILS_ASSERT(0);
    }
}

int wifimon_update(wifimon_t *pstate)
{
    wifi_state_t next_state;
    next_state = do_update(pstate);

    do_transitions(pstate, next_state);
    do_steadystate(pstate);

    return (pstate->curr_state==WM_CONNECTED); // return connection state
}

void wifimon_force_transition(wifimon_t *pstate, wifi_state_t next_state)
{
    pstate->debug_next_state = next_state;
}
