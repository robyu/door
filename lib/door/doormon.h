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
    DM_DOOR_CLOSING,
    DM_LAST_DONT_USE
} doormon_state_t;

typedef struct
{
    switch_t door_sensor0;
    switch_t test_button;
    int led_pin;
    doormon_state_t curr_state;
    long event_time_ms;
    int led_state;
} doormon_t;

#define DOORMON_DOOR_TRANSITION_MS    (5 * 1000)
void doormon_init(doormon_t *pstate, int door_sensor_pin0, int test_button_pin, int led_pin);
doormon_state_t doormon_update(doormon_t *pstate);

const char* doormon_state_to_string(doormon_state_t state);
const char* doormon_get_curr_state_as_string(const doormon_t *pstate);

#endif // doormon_h

