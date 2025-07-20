#include "Scheduler.h"

const int INVERTING_LOGIC = true;

Scheduler::Scheduler() {
    //default constructor
}
Scheduler::Scheduler(DeviceSettings& settings) {
    this->settings = settings;
}
void Scheduler::updateSchedule(const DeviceSettings& settings) {
    this->settings = settings;
    Serial.println("[Scheduler] Schedule updated with new settings.");
}

int Scheduler::timeToMinutes(int hour, int minute) {
    return hour * 60 + minute;
}

std::vector<SchedulerAction> Scheduler::checkSchedule(int currentHour, int currentMinute) {
    std::vector<SchedulerAction> actions;
    // Log when the function is called and with what parameters
    for (ChannelSetting &channel : settings.channels) {
        if(channel.scheduleEnabled) {
            Serial.printf("[Scheduler] Checking schedule for channel: %s\n", channel.pin.c_str());
            Serial.println("--- Scheduler Check ---");
            Serial.print("[Scheduler] Current Time: ");
            Serial.print(currentHour);
            Serial.print(":");
            if(currentMinute < 10) Serial.print("0"); // For leading zero
            Serial.print(currentMinute);
            
            Serial.print(" | Current State: ");
            bool effectiveState = INVERTING_LOGIC ? !channel.state : channel.state;
            Serial.println(effectiveState ? "ON" : "OFF");

            int _startHour = channel.startTime.substring(0, 2).toInt();
            int _startMinute = channel.startTime.substring(3, 5).toInt();
            int _endHour = channel.endTime.substring(0, 2).toInt();
            int _endMinute = channel.endTime.substring(3, 5).toInt();

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
                SchedulerAction action;
                action.channel = channel.pin;
                action.stateOnOFF = true;
                action.brightness = channel.sheduledBrightness; // Use scheduled brightness
                actions.push_back(action);
            } else {
                Serial.println("[Scheduler] Result: State mismatch. Sending TURN_OFF.");
                SchedulerAction action;
                action.channel = channel.pin;
                action.stateOnOFF = false;
                action.brightness = channel.sheduledBrightness;
                actions.push_back(action);
            }
        } else {
            //Serial.println("[Scheduler] Result: Schedule disabled. No action taken.");
        }
    }


    return actions;
}