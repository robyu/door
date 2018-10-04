


#include "Arduino.h"
#include "WiFiClient.h"
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <assert.h>
#include "wifimon.h"
#include "switch.h"
#include "tx.h"
#include "utils.h"

#define DEBUG 1
#define PRETEND_NETWORK_CONNECTED 0
#define LED_COUNTER_THRESHOLD 2

static int wifi_is_connected(void)
{
    wl_status_t status;
#if PRETEND_NETWORK_CONNECTED==0
    status = WiFi.status();
#else
    // 
    // pretend that network is always connected
    status = WL_CONNECTED;
#endif
    return (status==WL_CONNECTED);
}

void wifimon_init(wifimon_t *pstate, int led_pin, int reset_button_pin)
{
    memset(pstate, 0, sizeof(*pstate));
    pstate->led_pin = led_pin;

    if (led_pin >= 0)
    {
        pinMode(led_pin, OUTPUT);
    }
    switch_init(&pstate->reset_button, reset_button_pin);

    pstate->curr_state = WM_INIT;
    pstate->threshold_check_reset_ms = 5000;
    pstate->threshold_reboot_button_ms = 5000;
    pstate->threshold_reconfig_sec = 180; 
    pstate->threshold_not_connected_ms = 20000;
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

#ifdef DEBUG
    Serial.println("do_update: initial state: " + String(pstate->curr_state));
#endif

    next_state = pstate->curr_state;
    switch(pstate->curr_state)
    {
    case WM_INIT:
        next_state = WM_CHECK_RESET;
        break;

    case WM_CHECK_RESET:
#ifdef DEBUG
        Serial.println("WM_CHECK_RESET_STATE");
        Serial.println("  switch = " + String(switch_update_state(&pstate->reset_button)));
#endif
        if ((switch_update_state(&pstate->reset_button)==0) &&
            (utils_get_elapsed_msec_and_reset(&pstate->start_time) > 500)) 
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
#ifdef DEBUG
        Serial.println("WiFi.status = " + String(WiFi.status()));
#endif
        if (wifi_is_connected()==1)
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
#ifdef DEBUG
        Serial.println("WiFi.status = " + String(WiFi.status()));
#endif
        if (wifi_is_connected()==0)
        {
            // transition: turn off indicator LED
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
    default:
        Serial.println("wifimon::do_update illegal state " + String(pstate->curr_state));
        utils_restart();
    }

#ifdef DEBUG
    Serial.println("next_state: " + String(next_state));
#endif
    return next_state;
}

static void start_config_portal(void)
{
    // see https://github.com/tzapu/WiFiManager/blob/master/examples/OnDemandConfigPortal/OnDemandConfigPortal.ino
            
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
#ifdef DEBUG
    Serial.println("start_config_portal");
#endif
    //sets timeout until configuration portal gets turned off
    //useful to make it all retry or go to sleep
    //in seconds
    wifiManager.setTimeout(120);

    //it starts an access point with the specified name
    //and goes into a blocking loop awaiting configuration

    //WITHOUT THIS THE AP DOES NOT SEEM TO WORK PROPERLY WITH SDK 1.5 , update to at least 1.5.1
    //WiFi.mode(WIFI_STA);
    
    if (!wifiManager.startConfigPortal("door-config-ap")) {
        Serial.println("  failed to connect and hit timeout");
        delay(3000);
        //reset and try again, or maybe put it to deep sleep
        utils_restart();
    }

    //if you get here you have connected to the WiFi
    Serial.println("  connected...");
    utils_restart();
}

static void do_transitions(wifimon_t *pstate, wifi_state_t next_state)
{
    if (next_state==pstate->curr_state)
    {
        ; // do nothing
    }
    else
    {
#ifdef DEBUG
        Serial.println("do_transitions: " + String(pstate->curr_state) + "->" + String(next_state));
#endif
        switch(next_state)
        {
        case WM_CHECK_RESET:
            pstate->start_time = millis();
            break;
            
        case WM_RECONFIG:
            utils_set_led(pstate->led_pin, 1);
            start_config_portal();
            break;

        case WM_NOT_CONNECTED:
            pstate->start_time = millis();
            utils_set_led(pstate->led_pin, 0);
            break;

        case WM_CONNECTED:
            pstate->led_state = 1;
            //tx_send("connected_to_wifi","");
            break;

        case WM_REBOOT:
            utils_set_led(pstate->led_pin, 0);
            utils_restart();
            break;

        default:
            // shouldn't ever reach this point
            Serial.println("wifimon::do_transitions illegal state " + String(pstate->curr_state));
            utils_restart();
            
        }
        pstate->curr_state = next_state;
    }
}

static void do_steadystate(wifimon_t *pstate)
{
    switch(pstate->curr_state)
    {
    case WM_CHECK_RESET:
        pstate->led_counter++;
        utils_set_led(pstate->led_pin, pstate->led_counter % 2);
        break;

    case WM_CONNECTED:
        pstate->led_counter++;
#ifdef DEBUG
        //Serial.println("wifimon: led_counter=" + String(pstate->led_counter));
#endif
        if (pstate->led_counter >= LED_COUNTER_THRESHOLD)
        {
#ifdef DEBUG
            //Serial.println("wifimon: alternate LED state");
#endif
            pstate->led_state ^= 1;
            pstate->led_counter=0;
        }
        utils_set_led(pstate->led_pin, pstate->led_state);
        break;
    default:
        ;
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
