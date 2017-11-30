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

typedef struct
{
    wifi_state_t curr_state;
    int led_pin;
    int led_state;
    int led_counter;
    switch_t reset_button;
    long threshold_check_reset_ms;
    long threshold_reboot_button_ms;
    long threshold_reconfig_sec;
    long threshold_not_connected_ms;
    long start_time;
} wifimon_t;


void wifimon_init(wifimon_t *pstate, int led_pin, int reset_button_pin);
int wifimon_update(wifimon_t *pstate);

#endif // switch_h

