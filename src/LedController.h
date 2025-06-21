#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>

class LedController {
public:
    /**
     * @brief Constructor for the LedController.
     * @param pwmPin The GPIO pin number that the LED is connected to.
     * @param inverted Set to 'true' for active-low LEDs (where LOW turns the LED ON),
     * which is common. Set to 'false' for active-high LEDs.
     */
    LedController(int pwmPin, bool inverted = true);

    /**
     * @brief Initializes the LED pin and sets its initial state to OFF.
     * Call this inside your main setup() function.
     */
    void begin();

    /**
     * @brief Updates the LED's state and brightness.
     * @param state The desired state from your control logic. false (0) means ON, true means OFF.
     * @param brightness The desired brightness from 0 to 100. This is only applied when the state is ON.
     */
    void update(bool state, int brightness);

private:
    int _pin;
    bool _invertingLogic;
    // The ESP8266 has a 10-bit PWM resolution, so the range is 0-1023.
    const int PWM_RANGE = 1023;
};

#endif // LED_CONTROLLER_H