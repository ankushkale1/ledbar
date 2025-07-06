#include "Scheduler.h"

const int INVERTING_LOGIC = true;

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
    // Log when the function is called and with what parameters
    Serial.println("--- Scheduler Check ---");
    Serial.print("[Scheduler] Current Time: ");
    Serial.print(currentHour);
    Serial.print(":");
    if(currentMinute < 10) Serial.print("0"); // For leading zero
    Serial.print(currentMinute);
    Serial.print(" | Current State: ");
    bool effectiveState = INVERTING_LOGIC ? !currentLedState : currentLedState;
    Serial.println(effectiveState ? "ON" : "OFF");

    if (!_enabled) {
        Serial.println("[Scheduler] Result: Disabled. No action taken.");
        return NO_ACTION;
    }

    int nowInMinutes = timeToMinutes(currentHour, currentMinute);
    int startInMinutes = timeToMinutes(_startHour, _startMinute);
    int endInMinutes = timeToMinutes(_endHour, _endMinute);

    // Log the calculated time values for debugging
    Serial.print("[Scheduler] In Minutes -> Now: ");
    Serial.print(nowInMinutes);
    Serial.print(" | Start: ");
    Serial.print(startInMinutes);
    Serial.print(" | End: ");
    Serial.println(endInMinutes);

    bool shouldBeOn = false;

    if (startInMinutes <= endInMinutes) {
        // Normal day schedule (e.g., 08:00 to 17:00)
        if (nowInMinutes >= startInMinutes && nowInMinutes < endInMinutes) {
            shouldBeOn = true;
        }
        Serial.print("[Scheduler] Logic: Normal Day. Should be ON: ");
        Serial.println(shouldBeOn ? "Yes" : "No");
    } else {
        // Overnight schedule (e.g., 22:00 to 06:00)
        if (nowInMinutes >= startInMinutes || nowInMinutes < endInMinutes) {
            shouldBeOn = true;
        }
        Serial.print("[Scheduler] Logic: Overnight. Should be ON: ");
        Serial.println(shouldBeOn ? "Yes" : "No");
    }

    if (shouldBeOn) {
        Serial.println("[Scheduler] Result: State mismatch. Sending TURN_ON.");
        return TURN_ON;
    } else {
        Serial.println("[Scheduler] Result: State mismatch. Sending TURN_OFF.");
        return TURN_OFF;
    }

    Serial.println("[Scheduler] Result: State matches schedule. No action needed.");
    return NO_ACTION;
}