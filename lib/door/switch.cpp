#include "switch.h"
#include "Arduino.h"
#include <assert.h>
#include "debounce.h"



void switch_init(switch_t *pstate, int pin)
{
    memset(pstate, 0, sizeof(*pstate));
    pstate->pin = pin;

    Serial.println("switch_init: init pin " + String(pstate->pin));
    DebounceInit(&pstate->debouncer, pin);

    pstate->closed_state = LOW; // LOW is closed switch
}

/*
returns 1 if switch is closed
*/
int switch_update_state(switch_t *pstate)
{
    int switch_state;
    int switch_is_closed;

    switch_state = Debounce(&pstate->debouncer);
    if (switch_state!=pstate->curr_state)
    {
        Serial.println("switch state changed");
        pstate->curr_state = switch_state;
        pstate->state_change_time = millis();
    }
    switch_is_closed = (pstate->closed_state == pstate->curr_state);
    return switch_is_closed;
}

long switch_get_state_duration_ms(switch_t *pstate)
{
    long duration = millis() - pstate->state_change_time;
    if (duration < 0)
    {
        // duration rolled over, so reset state_change_time
        pstate->state_change_time = millis();
        duration = 0;
    }
    return duration;
}

void switch_reset_duration(switch_t *pstate)
{
    pstate->state_change_time = millis();
}
