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
    // these three inputs are equivalent
    switch_t door_sensor0; // 1
    switch_t test_button; // 2
    int test_flag; // 3

    long int transition_wait_ms;

    int led_pin;
    int led_state;
    doormon_state_t curr_state;
    long int transition_time_ms;  // the clock at time of state transition
    unsigned long state_elapsed_time_ms; // how long we've been in a particular state
} doormon_t;

#define DOORMON_DOOR_TRANSITION_WAIT_MS    (5 * 1000)
void doormon_init(doormon_t *pstate, int door_sensor_pin0, int test_button_pin, int led_pin);
doormon_state_t doormon_update(doormon_t *pstate);

const String doormon_state_to_string(doormon_state_t state);
const String doormon_get_curr_state_as_string(const doormon_t *pstate);

void doormon_set_test_flag(doormon_t *pstate, int val);
int doormon_get_test_flag(const doormon_t *pstate);
unsigned long doormon_get_elapsed_state_time_ms(const doormon_t *pstate);
#endif // doormon_h

