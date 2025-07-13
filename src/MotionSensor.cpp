#include "MotionSensor.h"

MotionSensor::MotionSensor(int pin) : _pin(pin) {}

void MotionSensor::begin() {
    pinMode(_pin, INPUT);
}

bool MotionSensor::motionDetected() {
    return digitalRead(_pin) == HIGH;
}