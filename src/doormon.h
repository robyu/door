#ifndef doormon_h
#define doormon_h

#include "Arduino.h"
#include "switch.h"

typedef enum
{
    DM_DONT_USE = 0,
    DM_INIT,   // initial state after reboot
    DM_DOOR_CLOSED,
    DM_DOOR_OPENING,
    DM_DOOR_OPEN,
    DM_LAST_DONT_USE
} doormon_state_t;

typedef struct
{
    switch_t door_sensor0;
    switch_t door_sensor1;
    switch_t test_button;
    int led_pin;
    doormon_state_t curr_state;
    long event_time_ms;
} doormon_t;

#define DOORMON_ALERT_INTERVAL_MS  (1 * 60 * 1000)
#define DOORMON_DOOR_OPENING_MS    (5 * 1000)
void doormon_init(doormon_t *pstate, int door_sensor_pin0, int door_sensor_pin1, int test_button_pin, int led_pin);
void doormon_update(doormon_t *pstate, int is_connected);

#endif // doormon_h

