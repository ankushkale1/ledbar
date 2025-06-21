#include "LedController.h"

LedController::LedController(int pwmPin) : _pin(pwmPin) {}

void LedController::begin() {
    pinMode(_pin, OUTPUT);
    analogWriteRange(PWM_RANGE); // Set the PWM range to 0-1023
    digitalWrite(_pin, LOW);     // Start with LED off
}

void LedController::update(bool state, int brightness) {
    if (!state) {
        analogWrite(_pin, 0); // Turn off completely
    } else {
        // Map brightness (0-100) to PWM range (0-1023)
        int dutyCycle = map(brightness, 0, 100, 0, PWM_RANGE);
        analogWrite(_pin, dutyCycle);
    }
}