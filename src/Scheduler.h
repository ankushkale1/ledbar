#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>

enum SchedulerAction {
    NO_ACTION,
    TURN_ON,
    TURN_OFF
};

class Scheduler {
public:
    Scheduler();
    void updateSchedule(bool enabled, const String& startTime, const String& endTime);
    SchedulerAction checkSchedule(int currentHour, int currentMinute, bool currentLedState);

private:
    bool _enabled;
    int _startHour;
    int _startMinute;
    int _endHour;
    int _endMinute;

    int timeToMinutes(int hour, int minute);
};

#endif