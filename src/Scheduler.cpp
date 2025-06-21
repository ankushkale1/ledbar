#include "Scheduler.h"

Scheduler::Scheduler() {
    _enabled = false;
    _startHour = 22; _startMinute = 0;
    _endHour = 6; _endMinute = 0;
}

void Scheduler::updateSchedule(bool enabled, const String& startTime, const String& endTime) {
    _enabled = enabled;
    
    int H_start = startTime.substring(0, 2).toInt();
    int M_start = startTime.substring(3, 5).toInt();
    _startHour = H_start;
    _startMinute = M_start;

    int H_end = endTime.substring(0, 2).toInt();
    int M_end = endTime.substring(3, 5).toInt();
    _endHour = H_end;
    _endMinute = M_end;
    
    Serial.printf(" Updated: Enabled=%d, Start=%02d:%02d, End=%02d:%02d\n", _enabled, _startHour, _startMinute, _endHour, _endMinute);
}

int Scheduler::timeToMinutes(int hour, int minute) {
    return hour * 60 + minute;
}

SchedulerAction Scheduler::checkSchedule(int currentHour, int currentMinute, bool currentLedState) {
    if (!_enabled) {
        return NO_ACTION;
    }

    int nowInMinutes = timeToMinutes(currentHour, currentMinute);
    int startInMinutes = timeToMinutes(_startHour, _startMinute);
    int endInMinutes = timeToMinutes(_endHour, _endMinute);

    bool shouldBeOn = false;

    if (startInMinutes <= endInMinutes) {
        // Normal day schedule (e.g., 08:00 to 17:00)
        if (nowInMinutes >= startInMinutes && nowInMinutes < endInMinutes) {
            shouldBeOn = true;
        }
    } else {
        // Overnight schedule (e.g., 22:00 to 06:00)
        if (nowInMinutes >= startInMinutes || nowInMinutes < endInMinutes) {
            shouldBeOn = true;
        }
    }

    if (shouldBeOn &&!currentLedState) {
        return TURN_ON;
    } else if (!shouldBeOn && currentLedState) {
        return TURN_OFF;
    }

    return NO_ACTION;
}