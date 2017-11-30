#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <assert.h>
#include "switch.h"
#include "wifimon.h"
#include "tx.h"
#include "doormon.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 16
#endif

/*
  Pin assignments
  D# = pin label on package
  gpio = label used by ardiuno framework
  
|  D# | gpio |
|   0 |   16 |
|   1 |   5  |
|   2 |    4 |
|   3 |    0 |
|   4 |   2  |
|   5 |   14 |
|   6 |   12 |
|   7 |   13 |
|   8 |   15 |
|  tx |    1 |

 */

// pin assignments use GPIO labels
#define BTN_RESET    14  
#define BTN_DOOR     12 
#define REED_SWITCH0 5   
#define REED_SWITCH1 4
#define LED_DOOR     2
#define LED_WIFI     13
typedef struct
{
    wifimon_t wifimon;
    doormon_t doormon;
} state_t;
state_t state;

void setup()
{
    Serial.begin(115200);
    delay(10); // need this?

    Serial.println("button setup");
    Serial.flush();

    wifimon_init(&state.wifimon, LED_WIFI, BTN_RESET);

    doormon_init(&state.doormon, REED_SWITCH0, REED_SWITCH1, BTN_DOOR, LED_DOOR);

    //pinMode(BTN_RESET, INPUT_PULLUP);
    Serial.println("setup complete");
    Serial.flush();
}

void loop()
{
    static int count = 0;

    // slow down update rate
    count++;
    if ((count % 20000)==0)
    {
        int is_connected;
        is_connected = wifimon_update(&state.wifimon);
        doormon_update(&state.doormon, is_connected);
    }
}

//handle diagnostic informations given by assertion and abort program execution:
void __assert(const char *__func, const char *__file, int __lineno, const char *__sexp) {
    // transmit diagnostic informations through serial link. 
    Serial.println(__func);
    Serial.println(__file);
    Serial.println(__lineno, DEC);
    Serial.println(__sexp);
    Serial.flush();
    // abort program execution.
    abort();
}
