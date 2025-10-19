#include "SettingsManager.h"
#include <ArduinoJson.h>
#include <ArduinoLog.h>

#define JSON_BUFFER_SIZE 2048 // more the channels greater the size, 1024 per 4 channels approx
const String default_mDNSName = "ledbar";

SettingsManager::SettingsManager()
{
    // Initialization is handled in begin() to ensure filesystem is ready.
}

void SettingsManager::begin()
{
    preferences.begin("settings", false);
    if (mountFS())
    {
        if (!loadMDNSName())
        {
            saveMDNSName(default_mDNSName); // Default mDNS name
        }
        if (!loadSettings())
        {
            Log.infoln("[Settings] No settings file found or file corrupted, creating default settings.");
            // Populate with some default channels if none exist to prevent empty settings
            if (settings.channels.empty())
            {
                ChannelSetting channel1;
                channel1.pin = "GPIO3";
                channel1.channelName = "UnderTable Light";
                channel1.state = false;
                channel1.scheduleEnabled = false;
                channel1.irCode = "EC13FB04";
                channel1.brightness = 80;
                settings.channels.push_back(channel1);

                ChannelSetting channel2;
                channel2.pin = "GPIO4";
                channel2.channelName = "LedBar";
                channel2.state = false;
                channel2.scheduleEnabled = false;
                channel2.irCode = "ED12FB04";
                channel2.brightness = 80;
                settings.channels.push_back(channel2);

                ChannelSetting channel3;
                channel3.pin = "GPIO5";
                channel3.channelName = "Background Light";
                channel3.state = false;
                channel3.scheduleEnabled = false;
                channel3.startTime = "19:00";
                channel3.endTime = "23:30";
                channel3.scheduledBrightness = 80;
                channel3.irCode = "EE11FB04";
                channel3.brightness = 80;
                settings.channels.push_back(channel3);

                ChannelSetting channel4;
                channel4.pin = "GPIO6";
                channel4.state = false;
                channel4.scheduleEnabled = false;
                channel4.irCode = "EB14FB04";
                channel4.brightness = 80;
                settings.channels.push_back(channel4);

                ChannelSetting channel5;
                channel5.pin = "GPIO7";
                channel5.state = false;
                channel5.scheduleEnabled = false;
                channel5.irCode = "EA15FB04";
                channel5.brightness = 80;
                settings.channels.push_back(channel5);

                ChannelSetting channel6;
                channel6.pin = "GPIO10";
                channel6.state = false;
                channel6.scheduleEnabled = false;
                channel6.irCode = "E916FB04";
                channel6.brightness = 80;
                settings.channels.push_back(channel6);
            }
            settings.gmtOffsetSeconds = 19800; // IST
            settings.irCodeBrightnessDown = "F40BFB04";
            settings.irCodeBrightnessUp = "55AAFB04";
            saveSettings();
        }
    }
    else
    {
        Log.infoln("[Settings] CRITICAL: Filesystem could not be mounted.");
    }
}

bool SettingsManager::loadSettings()
{
    File configFile = SPIFFS.open("/settings.json", "r");
    if (!configFile)
    {
        Log.infoln("[Settings] Failed to open config file for reading.");
        return false;
    }

    // Use a DynamicJsonDocument to parse the file.
    // Adjust the size if your settings object grows.
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (error)
    {
        Serial.print(F("[Settings] deserializeJson() failed: "));
        Log.infoln(error.c_str());
        return false;
    }

    // Load scheduler settings, providing defaults if keys are missing
    settings.gmtOffsetSeconds = doc["gmt_offset"] | 19800; // Default to IST if not present
    settings.irCodeBrightnessUp = doc["irCodeBrightnessUp"].as<String>();
    settings.irCodeBrightnessDown = doc["irCodeBrightnessDown"].as<String>();
    loadMDNSName();

    // Load channel settings
    settings.channels.clear(); // Clear existing channels before loading new ones
    JsonArray channelsArray = doc["channels"].as<JsonArray>();
    for (JsonObject channelJson : channelsArray)
    {
        ChannelSetting ch;
        ch.pin = channelJson["pin"].as<String>();
        ch.channelName = channelJson["channelName"].as<String>();
        ch.irCode = channelJson["irCode"].as<String>();
        ch.state = channelJson["state"];
        ch.brightness = channelJson["brightness"];
        ch.scheduleEnabled = doc["sch_en"] | true;
        ch.startTime = doc["sch_s"] | "19:00";
        ch.endTime = doc["sch_e"] | "23:30";
        ch.scheduledBrightness = doc["sch_brightness"] | 80;
        settings.channels.push_back(ch);
    }

    Log.infoln("[Settings] Settings loaded successfully.");
    return true;
}

bool SettingsManager::saveSettings()
{
    File configFile = SPIFFS.open("/settings.json", "w");
    if (!configFile)
    {
        Log.infoln("[Settings] Failed to open config file for writing.");
        return false;
    }

    DynamicJsonDocument doc(JSON_BUFFER_SIZE);

    // Save scheduler settings
    doc["gmt_offset"] = settings.gmtOffsetSeconds;
    doc["mDNSName"] = settings.mDNSName;
    doc["irCodeBrightnessUp"] = settings.irCodeBrightnessUp;
    doc["irCodeBrightnessDown"] = settings.irCodeBrightnessDown;
    saveMDNSName(settings.mDNSName);

    // Save channel settings
    JsonArray channels = doc.createNestedArray("channels");
    for (const auto &ch_setting : settings.channels)
    {
        JsonObject channel = channels.createNestedObject();
        channel["pin"] = ch_setting.pin;
        channel["channelName"] = ch_setting.channelName;
        channel["irCode"] = ch_setting.irCode;
        channel["state"] = ch_setting.state;
        channel["brightness"] = ch_setting.brightness;
        channel["sch_en"] = ch_setting.scheduleEnabled;
        channel["sch_s"] = ch_setting.startTime;
        channel["sch_e"] = ch_setting.endTime;
        channel["sch_brightness"] = ch_setting.scheduledBrightness;
    }

    if (serializeJson(doc, configFile) == 0)
    {
        Log.infoln(F("[Settings] Failed to write to config file."));
        configFile.close();
        return false;
    }

    configFile.close();
    Log.infoln("[Settings] Settings saved successfully.");
    return true;
}

DeviceSettings &SettingsManager::getSettings()
{
    return settings;
}

bool SettingsManager::loadMDNSName()
{
    String storedMDNSName = preferences.getString("mDNSName", default_mDNSName);
    settings.mDNSName = storedMDNSName;
    Log.infoln("[Settings] mDNS name loaded from preferences: %s", settings.mDNSName.c_str());
    return true;
}

void SettingsManager::saveMDNSName(const String &mDNSName)
{
    preferences.putString("mDNSName", mDNSName);
    Log.infoln("[Settings] mDNS name saved to preferences: %s", mDNSName.c_str());
}

bool SettingsManager::mountFS()
{
    if (!SPIFFS.begin(true))
    {
        Log.infoln("[Settings] Failed to mount file system. Formatting...");
        if (SPIFFS.format())
        {
            Log.infoln("[Settings] Filesystem formatted successfully.");
            return SPIFFS.begin(true);
        }
        else
        {
            Log.infoln("[Settings] Filesystem format failed.");
            return false;
        }
    }
    return true;
}