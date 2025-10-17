#include "LedController.h"
#include <ArduinoLog.h>

LedController::LedController(bool inverted) : _invertingLogic(inverted) {}

void LedController::begin()
{
    // Initialization is now handled in the update function
}

int LedController::pinNameToNumber(const String &pinName)
{
    // ESP32-C3 has GPIO 0-21 available, but some pins have special functions
    // GPIO 0-5: Available for general use
    // GPIO 6-7: Reserved for SPI flash
    // GPIO 8-10: Available for general use
    // GPIO 11-17: Available for general use
    // GPIO 18-21: Available for general use
    if (pinName == "GPIO0")
        return 0; // Available for general use
    if (pinName == "GPIO1")
        return 1; // Available for general use
    if (pinName == "GPIO2")
        return 2; // Available for general use
    if (pinName == "GPIO3")
        return 3; // Available for general use
    if (pinName == "GPIO4")
        return 4; // Available for general use
    if (pinName == "GPIO5")
        return 5; // Available for general use
    // Skip GPIO 6-7 as they're used for SPI flash
    if (pinName == "GPIO8")
        return 8; // Available for general use
    if (pinName == "GPIO9")
        return 9; // Available for general use
    if (pinName == "GPIO10")
        return 10; // Available for general use
    if (pinName == "GPIO11")
        return 11; // Available for general use
    if (pinName == "GPIO12")
        return 12; // Available for general use
    if (pinName == "GPIO13")
        return 13; // Available for general use
    if (pinName == "GPIO14")
        return 14; // Available for general use
    if (pinName == "GPIO15")
        return 15; // Available for general use
    if (pinName == "GPIO16")
        return 16; // Available for general use
    if (pinName == "GPIO17")
        return 17; // Available for general use
    if (pinName == "GPIO18")
        return 18; // Available for general use
    if (pinName == "GPIO19")
        return 19; // Available for general use
    if (pinName == "GPIO20")
        return 20; // Available for general use
    if (pinName == "GPIO21")
        return 21; // Available for general use
    return -1;     // Invalid pin name
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

        int channelNum;
        if (_pinChannelMap.find(pin) == _pinChannelMap.end())
        {
            if (_nextChannel < 16)
            {
                channelNum = _nextChannel++;
                _pinChannelMap[pin] = channelNum;
                ledcSetup(channelNum, 5000, PWM_RESOLUTION);
                ledcAttachPin(pin, channelNum);
            }
            else
            {
                Log.errorln("[LedCtrl] No more LEDC channels available.");
                continue;
            }
        }
        else
        {
            channelNum = _pinChannelMap[pin];
        }

        int brightness = channel.schedulerActive ? channel.scheduledBrightness : channel.brightness;
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
                ledcWrite(channelNum, dutyCycle);
            }
            else
            {
                // For active-high, 100% brightness is PWM 255.
                dutyCycle = map(clampedBrightness, 0, 100, 0, PWM_RANGE);
                Log.info("[LedCtrl] Inverting logic OFF. Writing analog value: %d\n", dutyCycle);
                ledcWrite(channelNum, dutyCycle);
            }
        }
        else
        { // If the channel should be OFF
            Log.infoln("[LedCtrl] Channel is OFF");
            // Set pin to the OFF state
            int offState = _invertingLogic ? PWM_RANGE : 0;
            Log.info("[LedCtrl] Writing digital value: %s\n", offState == HIGH ? "HIGH" : "LOW");
            ledcWrite(channelNum, offState);
        }
        Log.infoln("[LedCtrl] --- Channel Processing End ---");
    }
    Log.infoln("[LedCtrl] --- Update Function End ---");
}