#ifndef switch_h
#define switch_h

#include "debounce.h"

typedef struct
{
    int curr_state;
    int pin;
    int closed_state;
    long state_change_time;
    Debounce_t debouncer;
} switch_t;


void switch_init(switch_t *pstate, int pin);
int switch_update_state(switch_t *pstate);
long switch_get_state_duration(switch_t *pstate);
void switch_reset_duration(switch_t *pstate);

#endif // switch_h

