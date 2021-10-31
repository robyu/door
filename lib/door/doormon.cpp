

#include "Arduino.h"
#include <assert.h>
#include "doormon.h"
#include "switch.h"
#include "utils.h"

#define DEBUG 1

/*
  reed switch is 1 when door is closed
*/
#define DOOR_OPEN   0
#define DOOR_CLOSED 1

typedef struct 
{
    int value;
    const char *pvalue_str;
} state_enum_descr_t;

/*
table for converting state enums to strings
*/
#define STATE_ENUM_DECLARE(ENUM) {ENUM, #ENUM}
#define STATE_ENUM_END {0, NULL}
static state_enum_descr_t doormon_state_descr[] =
{
    STATE_ENUM_DECLARE(DM_DONT_USE),
    STATE_ENUM_DECLARE(DM_INIT),
    STATE_ENUM_DECLARE(DM_DOOR_CLOSED),
    STATE_ENUM_DECLARE(DM_DOOR_OPENING),
    STATE_ENUM_DECLARE(DM_DOOR_OPEN),
    STATE_ENUM_DECLARE(DM_DOOR_CLOSING),
    STATE_ENUM_DECLARE(DM_LAST_DONT_USE),
    STATE_ENUM_END
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
        if ((switch_update_state(&pstate->door_sensor0)==DOOR_OPEN) ||
            (switch_update_state(&pstate->test_button)==1))
        {
            next_state = DM_DOOR_OPENING;
        }
        break;

    case DM_DOOR_OPENING:
        if ((switch_update_state(&pstate->door_sensor0)==DOOR_OPEN) ||
            (switch_update_state(&pstate->test_button)==1)) 
        {
            if (utils_get_elapsed_msec_and_reset(&pstate->event_time_ms) > DOORMON_DOOR_TRANSITION_MS)
            {
#ifdef DEBUG                
                Serial.println("doormon: door is definitely open");
#endif
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
        if ((switch_update_state(&pstate->door_sensor0)==DOOR_CLOSED) &&
            (switch_update_state(&pstate->test_button)==0))
        {
            next_state = DM_DOOR_CLOSING;
        }
        else
        {
            // door sensors = open
            ;
        }
        break;

    case DM_DOOR_CLOSING:
        if ((switch_update_state(&pstate->door_sensor0)==DOOR_CLOSED) &&
            (switch_update_state(&pstate->test_button)==0)) 
        {
            if (utils_get_elapsed_msec_and_reset(&pstate->event_time_ms) > DOORMON_DOOR_TRANSITION_MS)
            {
#ifdef DEBUG                
                Serial.println("doormon: door is definitely closed");
#endif
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
        Serial.println(String("doormon:do_update illegal state ") + String(doormon_state_to_string(pstate->curr_state)));
        utils_restart();
    }

    return next_state;
}

static void do_steadystate(doormon_t *pstate)
{
    long elapsed_ms;
    switch(pstate->curr_state)
    {
    case DM_DOOR_CLOSED:
        pstate->led_state = 0;
        break;

    case DM_DOOR_OPENING:
        pstate->led_state ^= 1;   // blink
        break;
        
    case DM_DOOR_OPEN:
        pstate->led_state = 1;
        
        elapsed_ms = utils_get_elapsed_msec_and_reset(&pstate->event_time_ms);
#ifdef DEBUG
        Serial.println("door has been open " + String(elapsed_ms/1000.0) + " sec");
#endif        
        break;

    case DM_DOOR_CLOSING:
        pstate->led_state ^= 1;   // blink
        break;

    default:
        ;
    }
    utils_set_led(pstate->led_pin, pstate->led_state);
}

static void do_transitions(doormon_t *pstate, doormon_state_t next_state)
{
    if (next_state == pstate->curr_state)
    {
        ; // pass
    }
    else
    {
        Serial.println(String(doormon_state_to_string(pstate->curr_state)) + "-->" + String(doormon_state_to_string(next_state)));
        switch(next_state)
        {
        case DM_DOOR_CLOSED:
            if (pstate->curr_state==DM_DOOR_OPEN)
            {
            }
            break;
            
        case DM_DOOR_OPENING:
            pstate->event_time_ms = millis();
            break;
            
        case DM_DOOR_OPEN:
            pstate->event_time_ms = millis();
            break;
            
        case DM_DOOR_CLOSING:
            pstate->event_time_ms = millis();
            break;

        default:
            //
            // shouldn't ever get here!
            Serial.println("doormon:do_transitions illegal state " + String(doormon_state_to_string(pstate->curr_state)));
            utils_restart();
        }
        pstate->curr_state = next_state;
    }
}

doormon_state_t doormon_update(doormon_t *pstate)
{
    doormon_state_t next_state;
    // {
    //     int on_off;
    //     on_off =  (switch_update_state(&pstate->door_sensor0)==DOOR_OPEN) || 
    //         switch_update_state(&pstate->test_button);

    // }
    next_state = do_update(pstate);
    do_transitions(pstate, next_state);
    do_steadystate(pstate);
    return next_state;
}


const char*  doormon_state_to_string(doormon_state_t state)
{
    return doormon_state_descr[(int)state].pvalue_str;
}

const char* doormon_get_curr_state_as_string(const doormon_t *pstate)
{
    doormon_state_t curr_state = pstate->curr_state;
    return doormon_state_to_string(curr_state);
}
