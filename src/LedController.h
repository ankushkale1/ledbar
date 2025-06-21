#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>

class LedController {
public:
    LedController(int pwmPin);
    void begin();
    void update(bool state, int brightness); // Brightness 0-100

private:
    int _pin;
    const int PWM_RANGE = 1023; // ESP8266 default PWM range
};

#endif