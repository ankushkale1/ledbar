#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>
#include "SettingsManager.h" // For DeviceSettings

class SchedulerAction {
    public:
    SchedulerAction() : channel(-1), stateOnOFF(false), brightness(0) {}
    String channel;
    bool stateOnOFF;
    int brightness;
};

class Scheduler {
public:
    Scheduler();
    Scheduler(DeviceSettings& settings);
    void updateSchedule(const DeviceSettings& settings);
    std::vector<SchedulerAction> checkSchedule(int currentHour, int currentMinute);

private:
    DeviceSettings settings;
    int timeToMinutes(int hour, int minute);
};

#endif