
#ifndef UTILS_H
#define UTILS_H

#include <assert.h>


long utils_get_elapsed_msec_and_reset(long* pevent_time_ms);
void utils_set_led(int led_pin, int on_off);
void utils_log(const char *fmt, ... );
void utils_print_float(float value, int places);
void utils_restart(void);

#define UTILS_INT_ABS(x) ((x >= 0) ? x : -x)

void utils_assert(const char* pfilename, int line_number, int arg);
#define UTILS_ASSERT(arg)  utils_assert(__FILE__,__LINE__,arg)


#endif

