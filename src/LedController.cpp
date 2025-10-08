#include "LedController.h"
#include <ArduinoLog.h>

LedController::LedController(bool inverted) : _invertingLogic(inverted) {}

void LedController::begin()
{
    // Initialization of pins is now handled dynamically in update()
    // to support changing pin configurations.
    analogWriteFreq(PWM_FREQ);   // Set to 256Hz, tells how many pwm cycles per second
    analogWriteRange(PWM_RANGE); // tells the range of values for pwm, 0-100 here, ie each cycle is devided into 100 steps
}

int LedController::pinNameToNumber(const String &pinName)
{
    if (pinName == "D0")
        return D0;
    if (pinName == "D1")
        return D1;
    if (pinName == "D2")
        return D2;
    if (pinName == "D3")
        return D3;
    if (pinName == "D4")
        return D4;
    if (pinName == "D5")
        return D5;
    if (pinName == "D6")
        return D6;
    if (pinName == "D7")
        return D7;
    if (pinName == "D8")
        return D8;
    // Add other pin mappings as needed
    return -1; // Invalid pin name
}

void LedController::update(const DeviceSettings &settings)
{
    Log.infoln("[LedCtrl] --- Update Function Start ---");

    for (const auto &channel : settings.channels)
    {
        Log.info("[LedCtrl] Processing channel for pin name: %s\n", channel.pin.c_str());

        int pin = pinNameToNumber(channel.pin);
        if (pin == -1)
        {
            Log.info("[LedCtrl] ERROR: Invalid pin name: %s\n", channel.pin.c_str());
            continue; // Skip to the next channel
        }
        Log.info("[LedCtrl] Pin number: %d\n", pin);

        // Initialize pin mode if not already done.
        pinMode(pin, OUTPUT);

        int brightness = channel.schedulerActive ? channel.sheduledBrightness : channel.brightness;
        bool state = channel.schedulerActive ? true : channel.state;

        Log.info("[LedCtrl] Raw settings - State: %s, Brightness: %d\n", state ? "ON" : "OFF", brightness);

        // Constrain brightness to 0-100 range
        int clampedBrightness = constrain(brightness, 0, 100);
        Log.info("[LedCtrl] Constrained Brightness: %d\n", clampedBrightness);

        if (state)
        { // If the channel should be ON
            Log.infoln("[LedCtrl] Channel is ON");
            int dutyCycle;
            if (_invertingLogic)
            {
                // For active-low, 100% brightness is PWM 0, and 0% is PWM 255.
                dutyCycle = map(clampedBrightness, 0, 100, PWM_RANGE, 0);
                Log.info("[LedCtrl] Inverting logic ON. Writing analog value: %d\n", dutyCycle);
                analogWrite(pin, dutyCycle);
            }
            else
            {
                // For active-high, 100% brightness is PWM 255.
                dutyCycle = map(clampedBrightness, 0, 100, 0, PWM_RANGE);
                Log.info("[LedCtrl] Inverting logic OFF. Writing analog value: %d\n", dutyCycle);
                analogWrite(pin, dutyCycle);
            }
        }
        else
        { // If the channel should be OFF
            Log.infoln("[LedCtrl] Channel is OFF");
            // Set pin to the OFF state
            int offState = _invertingLogic ? PWM_RANGE : 0;
            Log.info("[LedCtrl] Writing digital value: %s\n", offState == HIGH ? "HIGH" : "LOW");
            analogWrite(pin, offState);
        }
        Log.infoln("[LedCtrl] --- Channel Processing End ---");
    }
    Log.infoln("[LedCtrl] --- Update Function End ---");
}