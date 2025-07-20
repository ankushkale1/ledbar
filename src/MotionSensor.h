#ifndef MOTION_SENSOR_H
#define MOTION_SENSOR_H

#include <Arduino.h>

class MotionSensor {
public:
    MotionSensor(int pin);
    void begin();
    bool motionDetected();
private:
    int _pin;
};

#endif // MOTION_SENSOR_H