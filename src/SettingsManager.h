#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <vector>

// A struct to hold settings for a single PWM channel
struct ChannelSetting
{
  String pin;
  String channelName;
  bool state;
  int brightness;
  bool scheduleEnabled = false;
  String startTime = "22:00";
  String endTime = "06:00";
  int sheduledBrightness = 0;   // Default brightness for scheduled mode
  bool schedulerActive = false; // Indicates if the scheduler is active for this channel
};

// The main settings struct for the device
struct DeviceSettings
{
  std::vector<ChannelSetting> channels;
  long gmtOffsetSeconds = 19800; // Default to IST (+5:30)
  String mDNSName = "ledbar";
  // Remove old single-channel properties like ledState, brightness
};

class SettingsManager
{
public:
  SettingsManager();
  void begin();
  bool loadSettings();
  bool saveSettings();
  DeviceSettings &getSettings();
  bool loadMDNSNameFromEEPROM();
  void saveMDNSNameToEEPROM(const String &mDNSName);

private:
  DeviceSettings settings;
  bool mountFS();
};

#endif