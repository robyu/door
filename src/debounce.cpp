#include "Arduino.h"
#include "utils.h"
#include "debounce.h"

void DebounceInit(Debounce_t *pState,
                  int buttonPin)
{
    memset(pState, 0, sizeof(*pState));
    pState->buttonPin = buttonPin;
    pState->buttonState = HIGH;   // current reading
    pState->lastButtonState = HIGH;
    pState->lastDebounceTime = 0;
    pState->debounceDelay = 50;

    pinMode(buttonPin, INPUT_PULLUP);
}

// 
// gotta hold the button down to register a press
// returns button state:  LOW or HIGH
//
// test:  wire JC1 to input pin
// on protoshield, button goes LOW when pressed
//
// on esp8266, btn will read LOW when pressed
int Debounce(Debounce_t *pState)
{
    int reading = digitalRead(pState->buttonPin);

    // check to see if you just pressed the button 
    // (i.e. the input went from LOW to HIGH),  and you've waited 
    // long enough since the last press to ignore any noise:  

    /* utils_log("entering debounce: lastButtonStat="); */
    /* print_button_state(lastButtonState); */
    /* utils_log("| buttonState="); */
    /* print_button_state(buttonState); */
    /* utils_log("\n"); */

    // If the switch changed, due to noise or pressing:
    if (reading != pState->lastButtonState) {
        // reset the debouncing timer
        /*
        utils_log("bounce\n");
        */
        pState->lastDebounceTime = millis();
    } 
  
    if ((millis() - pState->lastDebounceTime) > (long unsigned int)pState->debounceDelay) {
        // whatever the reading is at, it's been there for longer
        // than the debounce delay, so take it as the actual current state:

        // if the button state has changed:
        if (reading != pState->buttonState) {
            /*
            utils_log("state change\n");
            */
            pState->buttonState = reading;
        }
    }
    pState->lastButtonState = reading;
    return pState->buttonState;
}

void debounce_print_button_state(int state)
{
    if (HIGH==state)
    {
        utils_log("high");
    }
    else
    {
        utils_log("low");
    }
}
