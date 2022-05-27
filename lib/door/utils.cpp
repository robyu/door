#include <stdarg.h>
#include <stdlib.h>
#include <Arduino.h>
#include <limits.h>
#include "gpio_pins.h"
#include "utils.h"

/*
  reset the esp8266
  also make sure to pull D3 and D0 high
  and D8 pull down
  see https://github.com/esp8266/Arduino/issues/1017
*/
void utils_restart(void)
{
    Serial.printf("REBOOTING NOW ==========================================");
    delay(500);
    ESP.restart();
}
/*
on_off is 1 or 0
not HIGH or LOW
*/
void utils_set_led(int led_pin, int on_off)
{
    int on_value = -1;
    
    // agh, fucking LEDs have different polarities
    UTILS_ASSERT(LED_DOOR==LED_BUILTIN_ESP);
    switch(led_pin)
    {
    case LED_BUILTIN_PCB:
    case LED_DOOR:
        on_value = LOW;
        break;

    case LED_WIFI:
        on_value = HIGH;
        break;
    default:
        Serial.println("unknown LED pin " + String(led_pin));
        UTILS_ASSERT(0);
    }

    if (on_off==1)
    {
        digitalWrite(led_pin, on_value);
    }
    else
    {
        digitalWrite(led_pin, on_value ^ 0x1);
    }
}


/*
given baseline time (event_time_ms),
get elapsed time.
reset event_time_ms if the elapsed time wraps
*/
long utils_get_elapsed_msec_and_reset(long* pevent_time_ms)
{
    long elapsed_ms;
    elapsed_ms = millis() - *pevent_time_ms;
    if (elapsed_ms < 0)
    {
        elapsed_ms = LONG_MAX;
        *pevent_time_ms = millis(); // reset
    }
    return elapsed_ms;
}

// from http://playground.arduino.cc/Code/PrintFloats
// printFloat prints out the float 'value' rounded to 'places' places after the decimal point
void utils_print_float(float value, int places) {
    // this is used to cast digits 
    int digit;
    float tens = 0.1;
    int tenscount = 0;
    int i;
    float tempfloat = value;

    // make sure we round properly. this could use pow from <math.h>, but doesn't seem worth the import
    // if this rounding step isn't here, the value  54.321 prints as 54.3209

    // calculate rounding term d:   0.5/pow(10,places)  
    float d = 0.5;
    if (value < 0)
        d *= -1.0;
    // divide by ten for each decimal place
    for (i = 0; i < places; i++)
        d/= 10.0;    
    // this small addition, combined with truncation will round our values properly 
    tempfloat +=  d;

    // first get value tens to be the large power of ten less than value
    // tenscount isn't necessary but it would be useful if you wanted to know after this how many chars the number will take

    if (value < 0)
        tempfloat *= -1.0;
    while ((tens * 10.0) <= tempfloat) {
        tens *= 10.0;
        tenscount += 1;
    }


    // write out the negative if needed
    if (value < 0)
        Serial.print('-');

    if (tenscount == 0)
        Serial.print(0, DEC);

    for (i=0; i< tenscount; i++) {
        digit = (int) (tempfloat/tens);
        Serial.print(digit, DEC);
        tempfloat = tempfloat - ((float)digit * tens);
        tens /= 10.0;
    }

    // if no places after decimal, stop now and return
    if (places <= 0)
        return;

    // otherwise, write the point and continue on
    Serial.print('.');  

    // now write out each decimal place by shifting digits one by one into the ones place and writing the truncated value
    for (i = 0; i < places; i++) {
        tempfloat *= 10.0; 
        digit = (int) tempfloat;
        Serial.print(digit,DEC);  
        // once written, subtract off that digit
        tempfloat = tempfloat - (float) digit; 
    }

    //Serial.flush();  // don't flush because it screws up interrupt!!

}

//
// enabling serial output slows down everything, can screw up mic readings
#define ENABLE_OUTPUT 1

// printf-like fcn for debug
void utils_log(const char *fmt, ... )
{
#if ENABLE_OUTPUT
    char tmp[128]; // resulting string limited to 128 chars
    va_list args;
    va_start (args, fmt );
    vsnprintf(tmp, 128, fmt, args);
    va_end (args);
    Serial.print(tmp);
    //Serial.flush();  // don't flush because it screws up interrupt!!
#endif
}

void utils_assert(const char* pfilename, int line_number, int arg)
{
    int blink = 1;
    if (arg)
    {
        return;
    }

    {
        char tmp[128]; // resulting string limited to 128 chars
        char fmt[] = "assertion at %s:%d\n";
        sprintf(tmp, fmt, pfilename, line_number);
        Serial.print(tmp);
        Serial.flush();


    }

    pinMode(LED_DOOR, OUTPUT);
    pinMode(LED_BUILTIN_PCB, OUTPUT);
    pinMode(LED_WIFI, OUTPUT);

    while(1)
    {
        digitalWrite(LED_BUILTIN_ESP, blink);
        digitalWrite(LED_BUILTIN_PCB, blink);
        digitalWrite(LED_WIFI, blink);
        blink ^= 0x1;
        delay(250);
    }
}
