#include "Arduino.h"
#include <assert.h>
#include "doormon.h"
#include "switch.h"
#include "utils.h"
#include "enum_descr.h"

#define DEBUG 1

/*
  reed switch is 1 when door is closed
*/
#define DOOR_OPEN   0
#define DOOR_CLOSED 1

/*
table for converting state enums to strings
*/
static enum_descr_t doormon_state_descr[] =
{
    ENUM_DESCR_DECLARE(DM_DONT_USE),
    ENUM_DESCR_DECLARE(DM_INIT),
    ENUM_DESCR_DECLARE(DM_DOOR_CLOSED),
    ENUM_DESCR_DECLARE(DM_DOOR_OPENING),
    ENUM_DESCR_DECLARE(DM_DOOR_OPEN),
    ENUM_DESCR_DECLARE(DM_DOOR_CLOSING),
    ENUM_DESCR_DECLARE(DM_LAST_DONT_USE),
    ENUM_DESCR_END
};

/*
  door_sensor_pin:  pin attached to reed switch
  test_button_pin:  pin attached to test button
  led_pin: pin attached to indicator LED
*/
void doormon_init(doormon_t *pstate, 
                  int door_sensor_pin0, 
                  int test_button_pin, 
                  int led_pin)
{
    memset(pstate, 0, sizeof(*pstate));

    if (led_pin >= 0)
    {
        pinMode(led_pin, OUTPUT);
        pstate->led_pin = led_pin;
    }
    else
    {
        pstate->led_pin = -1;
    }
    switch_init(&pstate->door_sensor0, door_sensor_pin0);
    switch_init(&pstate->test_button, test_button_pin);

    pstate->transition_wait_ms = DOORMON_DOOR_TRANSITION_WAIT_MS;

    pstate->curr_state = DM_INIT;
}

/*
  determine state machine's next state
*/
static doormon_state_t do_update(doormon_t *pstate)
{
    doormon_state_t next_state = pstate->curr_state;
    switch(pstate->curr_state)
    {
    case DM_INIT:
        next_state = DM_DOOR_CLOSED;
        break;

    case DM_DOOR_CLOSED:
        if ((pstate->sensor_state == DOOR_OPEN) ||
            (pstate->test_button_state == 1) ||
            (pstate->test_flag) )
        {
            next_state = DM_DOOR_OPENING;
        }
        break;

    case DM_DOOR_OPENING:
        if ((pstate->sensor_state==DOOR_OPEN) ||
            (pstate->test_button_state == 1) ||
            (pstate->test_flag) ) 
        {
            if (utils_get_elapsed_msec_and_reset(&pstate->transition_time_ms) > pstate->transition_wait_ms)
            {
                next_state = DM_DOOR_OPEN;
            }
        }
        else
        {
            // door sensors = closed
            next_state = DM_DOOR_CLOSED;
        }
        break;

    case DM_DOOR_OPEN:
        if ((pstate->sensor_state==DOOR_CLOSED) &&
            (pstate->test_button_state == 0) &&
            (pstate->test_flag==0))
        {
            next_state = DM_DOOR_CLOSING;
        }
        break;

    case DM_DOOR_CLOSING:
        if ((pstate->sensor_state==DOOR_CLOSED) &&
            (pstate->test_button_state == 0) &&
            (pstate->test_flag==0) )
        {
            if (utils_get_elapsed_msec_and_reset(&pstate->transition_time_ms) > pstate->transition_wait_ms)
            {
                next_state = DM_DOOR_CLOSED;
            }
        }
        else
        {
            // door sensor = open
            next_state = DM_DOOR_OPEN;
        }
        break;
    default:
        //
        // shouldn't ever get here!
        Serial.println(String("doormon:do_update illegal state ") + doormon_state_to_string(pstate->curr_state));
        utils_restart();
    }

    if (next_state != pstate->curr_state)
    {
        Serial.printf("    doormon::do_update: transition (%s) -> (%s)\n",
                      doormon_state_to_string(pstate->curr_state).c_str(),
                      doormon_state_to_string(next_state).c_str() );

        // note time at transition
        pstate->transition_time_ms = millis();

        pstate->curr_state = next_state;
    }

    return next_state;
}

static void do_steadystate(doormon_t *pstate)
{
    pstate->state_elapsed_time_ms = utils_get_elapsed_msec_and_reset(&pstate->transition_time_ms);
    pstate->sensor_state = switch_update_state(&pstate->door_sensor0);
    pstate->test_button_state = switch_update_state(&pstate->test_button);

    switch(pstate->curr_state)
    {
    case DM_INIT:
        break;
        
    case DM_DOOR_CLOSED:
        pstate->led_state = 0;
        break;

    case DM_DOOR_OPENING:
        pstate->led_state ^= 1;   // blink
        break;
        
    case DM_DOOR_OPEN:
        pstate->led_state = 1;
        
        break;

    case DM_DOOR_CLOSING:
        pstate->led_state ^= 1;   // blink
        break;

    default:
        ;
    }
    utils_set_led(pstate->led_pin, pstate->led_state);
}

doormon_state_t doormon_update(doormon_t *pstate)
{
    doormon_state_t next_state;
    // {
    //     int on_off;
    //     on_off =  (switch_update_state(&pstate->door_sensor0)==DOOR_OPEN) || 
    //         switch_update_state(&pstate->test_button);

    // }
    do_steadystate(pstate);
    next_state = do_update(pstate);
    return next_state;
}


const String  doormon_state_to_string(doormon_state_t state)
{
    return String(doormon_state_descr[(int)state].pvalue_str);
}

const String doormon_get_curr_state_as_string(const doormon_t *pstate)
{
    doormon_state_t curr_state = pstate->curr_state;
    return doormon_state_to_string(curr_state);
}

void doormon_set_test_flag(doormon_t *pstate, int val)
{
    pstate->test_flag = val;
}

int doormon_get_test_flag(const doormon_t *pstate)
{
    return pstate->test_flag;
}

unsigned long doormon_get_elapsed_state_time_ms(const doormon_t *pstate)
{
    return pstate->state_elapsed_time_ms;
}
