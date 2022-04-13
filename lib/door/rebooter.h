#ifndef REBOOTER_H
#define REBOOTER_H

#include "Arduino.h"
#include "switch.h"

typedef struct
{
    switch_t reboot_button;
    int led_pin;
    long duration_thresh_ms;
} rebooter_t;

void rebooter_init(rebooter_t *p, int button_pin, int led_pin);

// returns 1 when reboot button has been long-pressed
int reboot_update_state(rebooter_t *p);  

// reboot immediately
void reboot_now(rebooter_t *p); 
#endif
