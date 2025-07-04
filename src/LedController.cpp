#include "LedController.h"

LedController::LedController(bool inverted) : _invertingLogic(inverted) {}

void LedController::begin() {
    // Initialization of pins is now handled dynamically in update()
    // to support changing pin configurations.
}

int LedController::pinNameToNumber(const String& pinName) {
    if (pinName == "D0") return D0;
    if (pinName == "D1") return D1;
    if (pinName == "D2") return D2;
    if (pinName == "D3") return D3;
    if (pinName == "D4") return D4;
    if (pinName == "D5") return D5;
    if (pinName == "D6") return D6;
    if (pinName == "D7") return D7;
    if (pinName == "D8") return D8;
    // Add other pin mappings as needed
    return -1; // Invalid pin name
}

void LedController::update(const DeviceSettings& settings) {
    for (const auto& channel : settings.channels) {
        int pin = pinNameToNumber(channel.pin);
        if (pin == -1) {
            Serial.printf("[LedCtrl] Invalid pin name: %s\n", channel.pin.c_str());
            continue;
        }

        // Initialize pin mode if not already done.
        // A more robust implementation might track initialized pins to avoid calling this every time.
        pinMode(pin, OUTPUT);

        int brightness = channel.brightness;
        bool state = channel.state;

        // Constrain brightness to 0-100 range
        if (brightness < 0) brightness = 0;
        if (brightness > 100) brightness = 100;

        if (state) { // If the channel should be ON
            // Map brightness (0-100) to PWM range (0-255)
            int pwmValue = map(brightness, 0, 100, 0, PWM_RANGE);

            if (_invertingLogic) {
                // For active-low, 100% brightness is PWM 0, and 0% is PWM 255.
                analogWrite(pin, PWM_RANGE - pwmValue);
            } else {
                // For active-high, 100% brightness is PWM 255.
                analogWrite(pin, pwmValue);
            }
        } else { // If the channel should be OFF
            // Set pin to the OFF state
            digitalWrite(pin, _invertingLogic ? HIGH : LOW);
        }
    }
}