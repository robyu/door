#ifndef wifimon_h
#define wifimon_h


#include "Arduino.h"
#include "switch.h"
#include <WiFiManager.h>

typedef enum
{
    WM_DONT_USE = 0,
    WM_INIT,   // initial state after reboot
    WM_CHECK_RECONFIG_BTN, // check for reset button
    WM_RECONFIG, // reconfigure wifi
    WM_NOT_CONNECTED, // wifi not connected
    WM_CONNECTED, // wifi connected
    WM_REBOOT,
    WM_LAST_DONT_USE
} wifimon_state_t;

#define WIFIMON_MAX_LEN_MQTT_SERVER 128

typedef struct
{
    wifimon_state_t curr_state;
    
    int led_pin;
    int led_state;
    int led_counter;
    switch_t reset_button;
    long threshold_check_reset_ms;
    long threshold_reconfig_button_ms;
    long threshold_reconfig_sec;
    long threshold_not_connected_ms;
    long start_time;
    int reconfig_loop_cnt;

    char pmqtt_server[WIFIMON_MAX_LEN_MQTT_SERVER];
    short mqtt_port;
    
    // for debugging
    wifimon_state_t debug_next_state;
    bool enable_restart; // for testing
    bool pretend_network_connected; // for testing
} wifimon_t;


void wifimon_init(wifimon_t *pstate, int led_pin, int reconfig_button_pin);
wifimon_state_t wifimon_update(wifimon_t *pstate);
void wifimon_force_transition(wifimon_t *pstate, wifimon_state_t next_state);
void wifimon_print_info(WiFiManager *wm, wifimon_t *pstate);
void wifimon_write_mqtt_params_to_file(const char *pmqtt_server,
                                       short mqtt_port);
void wifimon_read_mqtt_params_from_file(char *pmqtt_server,
                                        short *pmqtt_port);

#endif // switch_h

