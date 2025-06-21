#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <Arduino.h>
#include <LittleFS.h>

// Struct to hold all configurable settings
struct DeviceSettings {
    bool ledState;
    int brightness;
    bool scheduleEnabled;
    String startTime; // "HH:MM"
    String endTime;   // "HH:MM"
};

class SettingsManager {
public:
    SettingsManager();
    void begin();
    bool loadSettings();
    bool saveSettings();
    DeviceSettings& getSettings();

private:
    DeviceSettings settings;
    bool mountFS();
};

#endif