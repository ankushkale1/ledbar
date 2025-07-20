#include "LedController.h"

LedController::LedController(bool inverted) : _invertingLogic(inverted) {}

void LedController::begin() {
    // Initialization of pins is now handled dynamically in update()
    // to support changing pin configurations.
    analogWriteRange(PWM_RANGE);
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
    Serial.println("[LedCtrl] --- Update Function Start ---");

    for (const auto& channel : settings.channels) {
        Serial.printf("[LedCtrl] Processing channel for pin name: %s\n", channel.pin.c_str());

        int pin = pinNameToNumber(channel.pin);
        if (pin == -1) {
            Serial.printf("[LedCtrl] ERROR: Invalid pin name: %s\n", channel.pin.c_str());
            continue; // Skip to the next channel
        }
        Serial.printf("[LedCtrl] Pin number: %d\n", pin);

        // Initialize pin mode if not already done.
        pinMode(pin, OUTPUT);

        int brightness = channel.schedulerActive ? channel.sheduledBrightness : channel.brightness;
        bool state = channel.schedulerActive ? true : channel.state;

        Serial.printf("[LedCtrl] Raw settings - State: %s, Brightness: %d\n", state ? "ON" : "OFF", brightness);

        // Constrain brightness to 0-100 range
        int clampedBrightness = constrain(brightness, 0, 100);
        Serial.printf("[LedCtrl] Constrained Brightness: %d\n", clampedBrightness);


        if (state) { // If the channel should be ON
            Serial.println("[LedCtrl] Channel is ON");
            int dutyCycle;
            if (_invertingLogic) {
                // For active-low, 100% brightness is PWM 0, and 0% is PWM 255.
                dutyCycle = map(clampedBrightness, 0, 100, PWM_RANGE, 0);
                Serial.printf("[LedCtrl] Inverting logic ON. Writing analog value: %d\n", dutyCycle);
                analogWrite(pin, dutyCycle);
            } else {
                // For active-high, 100% brightness is PWM 255.
                dutyCycle = map(clampedBrightness, 0, 100, 0, PWM_RANGE);
                Serial.printf("[LedCtrl] Inverting logic OFF. Writing analog value: %d\n", dutyCycle);
                analogWrite(pin, dutyCycle);
            }
        } else { // If the channel should be OFF
            Serial.println("[LedCtrl] Channel is OFF");
            // Set pin to the OFF state
            int offState = _invertingLogic ? PWM_RANGE : 0;
            Serial.printf("[LedCtrl] Writing digital value: %s\n", offState == HIGH ? "HIGH" : "LOW");
            analogWrite(pin, offState);
        }
         Serial.println("[LedCtrl] --- Channel Processing End ---");
    }
     Serial.println("[LedCtrl] --- Update Function End ---");
}