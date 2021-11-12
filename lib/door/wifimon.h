#ifndef wifimon_h
#define wifimon_h

#include "Arduino.h"
#include "switch.h"

typedef enum
{
    WM_DONT_USE = 0,
    WM_INIT,   // initial state after reboot
    WM_CHECK_RESET, // check for reset button
    WM_RECONFIG, // reconfigure wifi
    WM_NOT_CONNECTED, // wifi not connected
    WM_CONNECTED, // wifi connected
    WM_REBOOT,
    WM_LAST_DONT_USE
} wifi_state_t;

#define WIFIMON_MAX_LEN_MQTT_SERVER 255
#define WIFIMON_MAX_LEN_MQTT_PORT 6

typedef struct
{
    wifi_state_t curr_state;
    wifi_state_t debug_next_state;
    
    int led_pin;
    int led_state;
    int led_counter;
    switch_t reset_button;
    long threshold_check_reset_ms;
    long threshold_reboot_button_ms;
    long threshold_reconfig_sec;
    long threshold_not_connected_ms;
    long start_time;

    char pmqtt_server[WIFIMON_MAX_LEN_MQTT_SERVER];
    char pmqtt_port[WIFIMON_MAX_LEN_MQTT_PORT];

    bool restart_after_config;
} wifimon_t;


void wifimon_init(wifimon_t *pstate, int led_pin, int reset_button_pin);
int wifimon_update(wifimon_t *pstate);
void wifimon_force_transition(wifimon_t *pstate, wifi_state_t next_state);
#endif // switch_h

