

#include "Arduino.h"
#include "WiFiClient.h"
#include <assert.h>
#include "doormon.h"
#include "switch.h"
#include "utils.h"
#include "tx.h"

#define DEBUG 1

/*
  reed switch is 1 when door is closed
*/
#define DOOR_OPEN   0
#define DOOR_CLOSED 1

/*
  door_sensor_pin:  pin attached to reed switch
  test_button_pin:  pin attached to test button
  led_pin: pin attached to indicator LED
*/
void doormon_init(doormon_t *pstate, 
                  int door_sensor_pin0, 
                  int door_sensor_pin1, 
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
    switch_init(&pstate->door_sensor1, door_sensor_pin1);
    switch_init(&pstate->test_button, test_button_pin);

    pstate->curr_state = DM_INIT;
}

/*
  determine state machine's next state
*/
static doormon_state_t do_update(doormon_t *pstate, int is_connected)
{
    doormon_state_t next_state = pstate->curr_state;
    switch(pstate->curr_state)
    {
    case DM_INIT:
        next_state = DM_DOOR_CLOSED;
        break;

    case DM_DOOR_CLOSED:
        if ((switch_update_state(&pstate->door_sensor0)==DOOR_OPEN) ||
            (switch_update_state(&pstate->door_sensor1)==DOOR_OPEN) ||            
            (switch_update_state(&pstate->test_button)==1))
        {
            next_state = DM_DOOR_OPENING;
        }
        break;

    case DM_DOOR_OPENING:
        if ((switch_update_state(&pstate->door_sensor0)==DOOR_OPEN) ||
            (switch_update_state(&pstate->door_sensor1)==DOOR_OPEN) ||
            (switch_update_state(&pstate->test_button)==1)) 
        {
            if (utils_get_elapsed_msec_and_reset(&pstate->event_time_ms) > DOORMON_DOOR_OPENING_MS)
            {
#ifdef DEBUG                
                Serial.println("doormon: door is definitely open");
#endif
                if (is_connected)
                {
                    tx_send("door just opened","");
                }
                next_state = DM_DOOR_OPEN;
            }
        }
        else
        {
            next_state = DM_DOOR_CLOSED;
        }
        break;

    case DM_DOOR_OPEN:
        if ((switch_update_state(&pstate->door_sensor0)==DOOR_CLOSED) &&
            (switch_update_state(&pstate->door_sensor1)==DOOR_CLOSED) &&
            (switch_update_state(&pstate->test_button)==0))
        {
            next_state = DM_DOOR_CLOSED;
        }
        break;

    default:
        //
        // shouldn't ever get here!
        Serial.println("doormon:do_update illegal state " + String(pstate->curr_state));
        utils_restart();
    }

    if (next_state != pstate->curr_state)
    {
#ifdef DEBUG
        Serial.println("doormon: transition curr_state=" + String(pstate->curr_state) + " next_state = " + String(next_state));
#endif
    }
    return next_state;
}

static void do_steadystate(doormon_t *pstate, int is_connected)
{
    long elapsed_ms;
    switch(pstate->curr_state)
    {
    case DM_DOOR_CLOSED:
        utils_set_led(pstate->led_pin, 0);
        break;

    case DM_DOOR_OPENING:
        utils_set_led(pstate->led_pin, 1);
        break;
        
    case DM_DOOR_OPEN:
        utils_set_led(pstate->led_pin, 1);
        
        elapsed_ms = utils_get_elapsed_msec_and_reset(&pstate->event_time_ms);
#ifdef DEBUG
        Serial.println("door has been open " + String(elapsed_ms/1000.0) + " sec");
#endif        
        if (elapsed_ms > DOORMON_ALERT_INTERVAL_MS)
        {
#ifdef DEBUG
            Serial.println("ALERT: door open " + String(elapsed_ms/1000.0));
#endif
            pstate->event_time_ms = millis();  // reset timer
            if (is_connected)
            {
                tx_send("door has been open", String(elapsed_ms/1000.0) + " sec");
            }
        }
        break;
    default:
        ;
    }
}

static void do_transitions(doormon_t *pstate, doormon_state_t next_state, int is_connected)
{
    if (next_state == pstate->curr_state)
    {
        ; // pass
    }
    else
    {
        switch(next_state)
        {
        case DM_DOOR_CLOSED:
            if (pstate->curr_state==DM_DOOR_OPEN)
            {
                if (is_connected)
                {
                    tx_send("door has closed","");
                }
            }
            break;
            
        case DM_DOOR_OPENING:
            pstate->event_time_ms = millis();
            break;
            
        case DM_DOOR_OPEN:
            pstate->event_time_ms = millis();
            break;
            
        default:
            //
            // shouldn't ever get here!
            Serial.println("doormon:do_transitions illegal state " + String(pstate->curr_state));
            utils_restart();
        }
        pstate->curr_state = next_state;
    }
}

void doormon_update(doormon_t *pstate, int is_connected)
{
    doormon_state_t next_state;
#ifdef ISTHISUSED
    {
        int on_off;
        on_off =  (switch_update_state(&pstate->door_sensor0)==DOOR_OPEN) || 
            (switch_update_state(&pstate->door_sensor1)==DOOR_OPEN) || 
            switch_update_state(&pstate->test_button);

    }
#endif
    next_state = do_update(pstate, is_connected);
    do_transitions(pstate, next_state, is_connected);
    do_steadystate(pstate, is_connected);
}
