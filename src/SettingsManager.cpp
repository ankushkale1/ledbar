#include "SettingsManager.h"
#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <EEPROM.h>
#include <string.h>

#define MDNS_NAME_EEPROM_ADDR 0
const String default_mDNSName = "ledbar";
#define JSON_BUFFER_SIZE 2048 // more the channels greater the size, 1024 per 4 channels approx

SettingsManager::SettingsManager()
{
    // Initialization is handled in begin() to ensure filesystem is ready.
    // Constructor is now empty. begin() must be called from setup().
}

void SettingsManager::begin()
{
    if (mountFS())
    {
        EEPROM.begin(512);
        if (!loadMDNSNameFromEEPROM())
        {
            saveMDNSNameToEEPROM(default_mDNSName); // Default mDNS name
        }
        if (!loadSettings())
        {
            Log.infoln("[Settings] No settings file found or file corrupted, creating default settings.");
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
    File configFile = LittleFS.open("/settings.json", "r");
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
    settings.irCodeBrightnessUp = doc["irCodeBrightnessUp"] | "";
    settings.irCodeBrightnessDown = doc["irCodeBrightnessDown"] | "";
    loadMDNSNameFromEEPROM();

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
    File configFile = LittleFS.open("/settings.json", "w");
    if (!configFile)
    {
        Log.infoln("[Settings] Failed to open config file for writing.");
        return false;
    }

    DynamicJsonDocument doc(JSON_BUFFER_SIZE);

    // Save scheduler settings
    doc["gmt_offset"] = settings.gmtOffsetSeconds;
    doc["irCodeBrightnessUp"] = settings.irCodeBrightnessUp;
    doc["irCodeBrightnessDown"] = settings.irCodeBrightnessDown;
    saveMDNSNameToEEPROM(settings.mDNSName);

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

bool SettingsManager::loadMDNSNameFromEEPROM()
{
    String storedMDNSName = "";
    for (int i = 0; i < 32; i++)
    { // Assuming a max length of 32 for mDNS name
        char c = EEPROM.read(MDNS_NAME_EEPROM_ADDR + i);
        if (c == '\0' || c == 0xFF)
        { // Stop on null terminator or erased EEPROM byte
            break;
        }
        storedMDNSName += c;
    }

    // Stricter validation for mDNS hostnames
    bool nameIsValid = storedMDNSName.length() > 0 && storedMDNSName.length() < 32 && storedMDNSName.isEmpty() == false;
    if (nameIsValid)
    {
        // Hostnames cannot start or end with a hyphen
        if (storedMDNSName.startsWith("-") || storedMDNSName.endsWith("-"))
        {
            nameIsValid = false;
        }
        else
        {
            for (char c : storedMDNSName)
            {
                // Hostnames can only contain alphanumeric characters and hyphens
                if (!isalnum(c) && c != '-')
                {
                    nameIsValid = false;
                    break;
                }
            }
        }
    }

    if (nameIsValid)
    {
        settings.mDNSName = storedMDNSName;
        Log.infoln("[Settings] mDNS name loaded from EEPROM: %s", settings.mDNSName.c_str());
        return true;
    }
    else
    {
        settings.mDNSName = default_mDNSName; // Default if EEPROM is empty or invalid
        Log.infoln("[Settings] No mDNS name found in EEPROM, using default: %s", settings.mDNSName.c_str());
        saveMDNSNameToEEPROM(settings.mDNSName); // Correct the value in EEPROM for next boot
        return false;
    }
}

void SettingsManager::saveMDNSNameToEEPROM(const String &mDNSName)
{

    for (int i = 0; i < 32; i++)
    {
        EEPROM.write(MDNS_NAME_EEPROM_ADDR + i, i < mDNSName.length() ? mDNSName[i] : '\0');
    }
    bool commit_result = EEPROM.commit();
    if (commit_result)
    {
        Log.infoln("[Settings] mDNS name saved to EEPROM: %s", mDNSName.c_str());
    }
    else
    {
        Log.errorln("[Settings] EEPROM commit failed!");
    }
}

bool SettingsManager::mountFS()
{
    if (!LittleFS.begin())
    {
        Log.infoln("[Settings] Failed to mount file system. Formatting...");
        if (LittleFS.format())
        {
            Log.infoln("[Settings] Filesystem formatted successfully.");
            return LittleFS.begin();
        }
        else
        {
            Log.infoln("[Settings] Filesystem format failed.");
            return false;
        }
    }
    return true;
}