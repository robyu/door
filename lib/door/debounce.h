
#ifndef DEBOUNCE_H
#define DEBOUNCE_H
typedef struct
{
    int buttonPin;
    int buttonState;
    int lastButtonState;
    long lastDebounceTime;
    long debounceDelay;
} Debounce_t;

void DebounceInit(Debounce_t *pState,
                  int buttonPin);
int Debounce(Debounce_t *pState);

#endif


