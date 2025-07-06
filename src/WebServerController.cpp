#include "WebServerController.h"
#include "LittleFS.h"
#include <ArduinoJson.h>

WebServerController::WebServerController(int port, SettingsManager& settingsMgr, LedController& ledCtrl, Scheduler& scheduler, TimeManager& timeMgr)
    : _server(port), _settingsManager(settingsMgr), _ledController(ledCtrl), _scheduler(scheduler), _timeManager(timeMgr) {}

void WebServerController::begin() {
    _server.on("/", HTTP_GET, [this]() { this->handleRoot(); });
    _server.on("/settings", HTTP_POST, [this]() { this->handleSettings(); });
    _server.on("/status", HTTP_GET, [this]() { this->handleStatus(); });
    _server.onNotFound([this]() { this->handleNotFound(); });

    _server.begin();
    Serial.println("[Web] HTTP server started.");
}

void WebServerController::handleClient() {
    _server.handleClient();
}

void WebServerController::handleRoot() {
    File file = LittleFS.open("/index.html", "r");
    if (!file) {
        handleNotFound();
        return;
    }
    _server.streamFile(file, "text/html");
    file.close();
}

void WebServerController::handleSettings() {
    // The web server library puts the JSON payload in the "plain" argument
    if (_server.hasArg("plain") == false) {
        _server.send(400, "application/json", "{\"error\":\"Body required\"}");
        return;
    }

    String body = _server.arg("plain");
    DynamicJsonDocument doc(1024); // Adjust size as needed for your payload
    DeserializationError error = deserializeJson(doc, body);

    if (error) {
        Serial.print(F("[Web] deserializeJson() failed: "));
        Serial.println(error.c_str());
        _server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    DeviceSettings& settings = _settingsManager.getSettings();

    // Update scheduler settings from JSON
    settings.scheduleEnabled = doc["sch_en"];
    settings.startTime = doc["sch_s"].as<String>();
    settings.endTime = doc["sch_e"].as<String>();
    settings.gmtOffsetSeconds = doc["gmt_offset"] | 19800; // Use default if missing

    // Update channel settings from JSON
    JsonArray channelsArray = doc["channels"].as<JsonArray>();
    settings.channels.clear(); // Clear old channels before adding new ones
    for (JsonObject channelJson : channelsArray) {
        ChannelSetting ch;
        ch.pin = channelJson["pin"].as<String>();
        ch.state = channelJson["state"];
        ch.brightness = channelJson["brightness"];
        settings.channels.push_back(ch);
    }

    // Apply the new settings
    _ledController.update(settings); // Note: LedController needs refactoring for multi-channel
    _timeManager.setTimezone(settings.gmtOffsetSeconds);
    _scheduler.updateSchedule(settings.scheduleEnabled, settings.startTime, settings.endTime);
    _settingsManager.saveSettings();

    _server.send(200, "application/json", "{\"success\":true}");
}

void WebServerController::handleStatus() {
    DeviceSettings& settings = _settingsManager.getSettings();
    DynamicJsonDocument doc(1024); // Adjust size as needed

    doc["sch_en"] = settings.scheduleEnabled;
    doc["sch_s"] = settings.startTime;
    doc["sch_e"] = settings.endTime;
    doc["gmt_offset"] = settings.gmtOffsetSeconds;

    JsonArray channels = doc.createNestedArray("channels");
    for (const auto& ch_setting : settings.channels) {
        JsonObject channel = channels.createNestedObject();
        channel["pin"] = ch_setting.pin;
        channel["state"] = ch_setting.state;
        channel["brightness"] = ch_setting.brightness;
    }

    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void WebServerController::handleNotFound() {
    _server.send(404, "text/plain", "404: Not Found");
}