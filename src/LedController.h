#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>
#include "SettingsManager.h" // For DeviceSettings
#include <map>

class LedController
{
public:
    /**
     * @brief Constructor for the LedController.
     * @param inverted Set to 'true' for active-low LEDs (where LOW turns the LED ON),
     * which is common. Set to 'false' for active-high LEDs.
     */
    LedController(bool inverted = true);

    /**
     * @brief Initializes the LED controller.
     * Call this inside your main setup() function.
     */
    void begin();

    /**
     * @brief Updates all LED channels based on the provided settings.
     * @param settings The device settings containing all channel configurations.
     */
    void update(const DeviceSettings &settings);

private:
    bool _invertingLogic;
    std::map<int, int> _pinChannelMap;
    int _nextChannel = 0;
    const int PWM_RESOLUTION = 8;
    const int PWM_RANGE = 255;
    int pinNameToNumber(const String &pinName);
};

#endif // LED_CONTROLLER_H