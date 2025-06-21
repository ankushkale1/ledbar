#include "LedController.h"
#include <Arduino.h> // Required for pinMode, analogWrite, etc.

/**
 * @brief Constructor
 */
LedController::LedController(int pwmPin, bool inverted)
    : _pin(pwmPin), _invertingLogic(inverted) {}

/**
 * @brief Initializes the pin
 */
void LedController::begin() {
    pinMode(_pin, OUTPUT);
    analogWriteRange(PWM_RANGE);
    
    // Start with the LED in the OFF state for consistency.
    // For active-low (inverted), OFF is HIGH (PWM_RANGE).
    // For active-high (normal), OFF is LOW (0).
    int offValue = _invertingLogic ? PWM_RANGE : 0;
    analogWrite(_pin, offValue);
}

/**
 * @brief Updates the LED state and brightness based on your logic.
 */
void LedController::update(bool state, int brightness) {
    if (state) {
        // --- The command is to turn the LED ON ---

        // First, ensure the brightness value is safely within the 0-100 range.
        int clampedBrightness = constrain(brightness, 0, 100);

        int dutyCycle;
        if (_invertingLogic) {
            // For ACTIVE-LOW logic, the PWM signal is inverted.
            // Brightness 100% = Duty Cycle 0 (LOW)
            // Brightness 0%   = Duty Cycle PWM_RANGE (HIGH)
            dutyCycle = map(clampedBrightness, 0, 100, PWM_RANGE, 0);
        } else {
            // For ACTIVE-HIGH logic, the PWM signal is direct.
            // Brightness 100% = Duty Cycle PWM_RANGE (HIGH)
            // Brightness 0%   = Duty Cycle 0 (LOW)
            dutyCycle = map(clampedBrightness, 0, 100, 0, PWM_RANGE);
        }
        analogWrite(_pin, dutyCycle);

    } else {
        // --- The command is to turn the LED OFF ---
        
        // Determine the correct PWM value for the OFF state based on the hardware logic.
        int offValue = _invertingLogic ? PWM_RANGE : 0;
        analogWrite(_pin, offValue);
    }
}