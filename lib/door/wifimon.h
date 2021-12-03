#ifndef wifimon_h
#define wifimon_h


#include "Arduino.h"
#include "switch.h"
#include <WiFiManager.h>

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
    
    int led_pin;
    int led_counter;
    switch_t reset_button;
    long threshold_check_reset_ms;
    long threshold_reboot_button_ms;
    long threshold_reconfig_sec;
    long threshold_not_connected_ms;
    long start_time;

    char pmqtt_server[WIFIMON_MAX_LEN_MQTT_SERVER];
    char pmqtt_port[WIFIMON_MAX_LEN_MQTT_PORT];

    
    // for debugging
    wifi_state_t debug_next_state;
    bool enable_restart; // for testing
    bool pretend_network_connected; // for testing
} wifimon_t;


void wifimon_init(wifimon_t *pstate, int led_pin, int reset_button_pin);
int wifimon_update(wifimon_t *pstate);
void wifimon_force_transition(wifimon_t *pstate, wifi_state_t next_state);

void wifimon_write_mqtt_params_to_file(char *pmpqtt_server,
                                       int len_server,
                                       char *mpqtt_port,
                                       int len_port);

void wifimon_read_mqtt_params_from_file(char *pmpqtt_server,
                                        int len_server,
                                        char *mpqtt_port,
                                        int len_port);

#endif // switch_h

