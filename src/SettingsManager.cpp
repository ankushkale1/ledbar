#include "SettingsManager.h"

SettingsManager::SettingsManager() {
    // Initialize with safe default values
    settings.ledState = false;
    settings.brightness = 100;
    settings.scheduleEnabled = false;
    settings.startTime = "20:00";
    settings.endTime = "06:00";
}

void SettingsManager::begin() {
    if (mountFS()) {
        if (!loadSettings()) {
            // If settings file doesn't exist or is corrupt, save defaults
            saveSettings();
        }
    } else {
        Serial.println(" Failed to mount LittleFS. Using default settings.");
    }
}

bool SettingsManager::mountFS() {
    if (!LittleFS.begin()) {
        Serial.println(" An error occurred while mounting LittleFS.");
        return false;
    }
    return true;
}

bool SettingsManager::loadSettings() {
    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile) {
        Serial.println(" Failed to open config file for reading. It might not exist yet.");
        return false;
    }

    size_t size = configFile.size();
    if (size > 1024) {
        Serial.println(" Config file size is too large.");
        return false;
    }

    String content = configFile.readString();
    configFile.close();

    // Simple manual parsing to avoid large JSON library dependency
    // This is a basic implementation; a robust parser would be better for complex files.
    if (content.indexOf("\"ledState\":")!= -1) {
        settings.ledState = (content.indexOf("\"ledState\":true")!= -1);
    }
    if (content.indexOf("\"brightness\":")!= -1) {
        int start = content.indexOf("\"brightness\":") + 13;
        int end = content.indexOf(",", start);
        settings.brightness = content.substring(start, end).toInt();
    }
    if (content.indexOf("\"scheduleEnabled\":")!= -1) {
        settings.scheduleEnabled = (content.indexOf("\"scheduleEnabled\":true")!= -1);
    }
    if (content.indexOf("\"startTime\":")!= -1) {
        int start = content.indexOf("\"startTime\":\"") + 13;
        int end = content.indexOf("\"", start);
        settings.startTime = content.substring(start, end);
    }
    if (content.indexOf("\"endTime\":")!= -1) {
        int start = content.indexOf("\"endTime\":\"") + 11;
        int end = content.indexOf("\"", start);
        settings.endTime = content.substring(start, end);
    }

    Serial.println(" Settings loaded successfully.");
    return true;
}

bool SettingsManager::saveSettings() {
    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
        Serial.println(" Failed to open config file for writing.");
        return false;
    }

    // Manual JSON creation
    String content = "{";
    content += "\"ledState\":" + String(settings.ledState? "true" : "false") + ",";
    content += "\"brightness\":" + String(settings.brightness) + ",";
    content += "\"scheduleEnabled\":" + String(settings.scheduleEnabled? "true" : "false") + ",";
    content += "\"startTime\":\"" + settings.startTime + "\",";
    content += "\"endTime\":\"" + settings.endTime + "\"";
    content += "}";

    configFile.print(content);
    configFile.close();

    Serial.println(" Settings saved.");
    return true;
}

DeviceSettings& SettingsManager::getSettings() {
    return settings;
}