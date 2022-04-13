#include "rebooter.h"
#include "utils.h"
void rebooter_init(rebooter_t *p, int button_pin, int led_pin)
{
    memset(p, 0, sizeof(*p));
    p->led_pin = led_pin;
    pinMode(p->led_pin, OUTPUT);
    p->duration_thresh_ms = 3 * 1000;
    switch_init(&p->reboot_button, button_pin);
}

// returns 1 when reboot button has been long-pressed
int reboot_update_state(rebooter_t *p)
{
    int btn_state;
    int longpress_flag;
    btn_state = switch_update_state(&p->reboot_button);
    utils_set_led(p->led_pin, btn_state==1);

    longpress_flag = btn_state && (switch_get_state_duration_ms(&p->reboot_button) > p->duration_thresh_ms);
    return longpress_flag;
}

// reboot immediately
void reboot_now(rebooter_t *p)
{
    utils_restart();
}

