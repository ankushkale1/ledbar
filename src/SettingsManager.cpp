#include "SettingsManager.h"
#include <ArduinoJson.h>

SettingsManager::SettingsManager()
{
    // Initialization is handled in begin() to ensure filesystem is ready.
}

void SettingsManager::begin()
{
    if (mountFS())
    {
        if (!loadSettings())
        {
            Serial.println("[Settings] No settings file found or file corrupted, creating default settings.");
            // Populate with some default channels if none exist to prevent empty settings
            if (settings.channels.empty())
            {
                settings.channels.push_back({"D1", false, 80});
                settings.channels.push_back({"D2", false, 80});
            }
            saveSettings();
        }
    }
    else
    {
        Serial.println("[Settings] CRITICAL: Filesystem could not be mounted.");
    }
}

bool SettingsManager::loadSettings()
{
    File configFile = LittleFS.open("/settings.json", "r");
    if (!configFile)
    {
        Serial.println("[Settings] Failed to open config file for reading.");
        return false;
    }

    // Use a DynamicJsonDocument to parse the file.
    // Adjust the size if your settings object grows.
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (error)
    {
        Serial.print(F("[Settings] deserializeJson() failed: "));
        Serial.println(error.c_str());
        return false;
    }

    // Load scheduler settings, providing defaults if keys are missing
    settings.gmtOffsetSeconds = doc["gmt_offset"] | 19800; // Default to IST if not present

    // Load channel settings
    settings.channels.clear(); // Clear existing channels before loading new ones
    JsonArray channelsArray = doc["channels"].as<JsonArray>();
    for (JsonObject channelJson : channelsArray)
    {
        ChannelSetting ch;
        ch.pin = channelJson["pin"].as<String>();
        ch.channelName = channelJson["channelName"].as<String>();
        ch.state = channelJson["state"];
        ch.brightness = channelJson["brightness"];
        ch.scheduleEnabled = doc["sch_en"] | true;
        ch.startTime = doc["sch_s"] | "19:00";
        ch.endTime = doc["sch_e"] | "23:30";
        ch.sheduledBrightness = doc["sch_brightness"] | 80;
        settings.channels.push_back(ch);
    }

    Serial.println("[Settings] Settings loaded successfully.");
    return true;
}

bool SettingsManager::saveSettings()
{
    File configFile = LittleFS.open("/settings.json", "w");
    if (!configFile)
    {
        Serial.println("[Settings] Failed to open config file for writing.");
        return false;
    }

    DynamicJsonDocument doc(1024);

    // Save scheduler settings
    doc["gmt_offset"] = settings.gmtOffsetSeconds;

    // Save channel settings
    JsonArray channels = doc.createNestedArray("channels");
    for (const auto &ch_setting : settings.channels)
    {
        JsonObject channel = channels.createNestedObject();
        channel["pin"] = ch_setting.pin;
        channel["channelName"] = ch_setting.channelName;
        channel["state"] = ch_setting.state;
        channel["brightness"] = ch_setting.brightness;
        channel["sch_en"] = ch_setting.scheduleEnabled;
        channel["sch_s"] = ch_setting.startTime;
        channel["sch_e"] = ch_setting.endTime;
        channel["sch_brightness"] = ch_setting.sheduledBrightness;
    }

    if (serializeJson(doc, configFile) == 0)
    {
        Serial.println(F("[Settings] Failed to write to config file."));
        configFile.close();
        return false;
    }

    configFile.close();
    Serial.println("[Settings] Settings saved successfully.");
    return true;
}

DeviceSettings &SettingsManager::getSettings()
{
    return settings;
}

bool SettingsManager::mountFS()
{
    if (!LittleFS.begin())
    {
        Serial.println("[Settings] Failed to mount file system. Formatting...");
        if (LittleFS.format())
        {
            Serial.println("[Settings] Filesystem formatted successfully.");
            return LittleFS.begin();
        }
        else
        {
            Serial.println("[Settings] Filesystem format failed.");
            return false;
        }
    }
    return true;
}