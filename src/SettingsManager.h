#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <vector>

// A struct to hold settings for a single PWM channel
struct ChannelSetting {
  String pin;
  bool state;
  int brightness;
};

// The main settings struct for the device
struct DeviceSettings {
  std::vector<ChannelSetting> channels;
  bool scheduleEnabled = false;
  String startTime = "22:00";
  String endTime = "06:00";
  // Remove old single-channel properties like ledState, brightness
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