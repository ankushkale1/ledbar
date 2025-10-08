#include "WebServerController.h"
#include "LittleFS.h"
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <ArduinoLog.h>

WebServerController::WebServerController(int port, WebSocketsServer &ws, SettingsManager &settingsMgr, LedController &ledCtrl, Scheduler &scheduler, TimeManager &timeMgr)
    : _server(port), _ws(ws), _settingsManager(settingsMgr), _ledController(ledCtrl), _scheduler(scheduler), _timeManager(timeMgr) {}

void WebServerController::begin()
{
    _server.on("/", HTTP_GET, [this]()
               { this->handleRoot(); });
    _server.on("/settings", HTTP_POST, [this]()
               { this->handleSettings(); });
    _server.on("/status", HTTP_GET, [this]()
               { this->handleStatus(); });
    _server.on("/version", HTTP_GET, [this]()
               { this->handleVersion(); });
    _server.onNotFound([this]()
                       { this->handleNotFound(); });

    _server.begin();
    _ws.begin();
    Log.infoln("[Web] HTTP and WebSocket server started.");
}

void WebServerController::handleClient()
{
    _server.handleClient();
}

void WebServerController::handleRoot()
{
    serveFile("/index.html");
}

void WebServerController::serveFile(const String &filePath)
{
    File file = LittleFS.open(filePath, "r");
    if (file)
    {
        _server.streamFile(file, getContentType(filePath));
        file.close();
    }
    else
        handleNotFound();
}

void WebServerController::handleSettings()
{
    // The web server library puts the JSON payload in the "plain" argument
    if (_server.hasArg("plain") == false)
    {
        _server.send(400, "application/json", "{\"error\":\"Body required\"}");
        return;
    }

    String body = _server.arg("plain");
    DynamicJsonDocument doc(1024); // Adjust size as needed for your payload
    DeserializationError error = deserializeJson(doc, body);

    if (error)
    {
        Serial.print(F("[Web] deserializeJson() failed: "));
        Log.infoln(error.c_str());
        _server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    DeviceSettings &settings = _settingsManager.getSettings();

    // Update scheduler settings from JSON
    settings.gmtOffsetSeconds = doc["gmt_offset"] | 19800; // Use default if missing

    String newMDNSName = doc["mDNSName"].as<String>();

    if (settings.mDNSName != newMDNSName)
    {
        // Check if the new mDNS name is already in use
        if (MDNS.queryService(newMDNSName, "tcp") > 0)
        {
            Log.infoln("[Web] mDNS name already in use.");
            _server.send(400, "application/json", "{\"error\":\"mDNS name already in use\"}");
            return;
        }

        Log.infoln("[Web] mDNS name changed. Restarting...");
        settings.mDNSName = newMDNSName;
        _settingsManager.saveSettings();
        ESP.restart();
    }
    // Update channel settings from JSON
    JsonArray channelsArray = doc["channels"].as<JsonArray>();
    settings.channels.clear(); // Clear old channels before adding new ones
    for (JsonObject channelJson : channelsArray)
    {
        ChannelSetting ch;
        ch.pin = channelJson["pin"].as<String>();
        ch.channelName = channelJson["channelName"].as<String>();
        ch.irCode = channelJson["irCode"].as<String>();
        ch.state = channelJson["state"];
        ch.brightness = channelJson["brightness"];
        ch.scheduleEnabled = channelJson["schedulerEnabled"];
        ch.startTime = channelJson["scheduler_start"].as<String>();
        ch.endTime = channelJson["scheduler_end"].as<String>();
        ch.scheduledBrightness = channelJson["scheduler_brightness"];
        settings.channels.push_back(ch);
    }

    // Apply the new settings
    _ledController.update(settings); // Note: LedController needs refactoring for multi-channel
    _timeManager.setTimezone(settings.gmtOffsetSeconds);
    _scheduler.updateSchedule(settings);
    _settingsManager.saveSettings();
    _server.send(200, "application/json", "{\"success\":true}");
}

void WebServerController::handleStatus()
{
    DeviceSettings &settings = _settingsManager.getSettings();
    DynamicJsonDocument doc(1024); // Adjust size as needed

    doc["gmt_offset"] = settings.gmtOffsetSeconds;
    doc["mDNSName"] = settings.mDNSName;

    JsonArray channels = doc.createNestedArray("channels");
    for (const auto &ch_setting : settings.channels)
    {
        JsonObject channel = channels.createNestedObject();
        channel["pin"] = ch_setting.pin;
        channel["irCode"] = ch_setting.irCode;
        channel["channelName"] = ch_setting.channelName;
        channel["state"] = ch_setting.state;
        channel["brightness"] = ch_setting.brightness;
        channel["schedulerEnabled"] = ch_setting.scheduleEnabled;
        channel["scheduler_start"] = ch_setting.startTime;
        channel["scheduler_end"] = ch_setting.endTime;
        channel["scheduler_brightness"] = ch_setting.scheduledBrightness;
    }

    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void WebServerController::handleVersion()
{
    DynamicJsonDocument doc(64);
    doc["version"] = APP_VERSION;
    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void WebServerController::handleNotFound()
{
    _server.send(404, "text/plain", "404: Not Found");
}

String WebServerController::getContentType(const String &filePath)
{
    if (filePath.endsWith(".html"))
        return "text/html";
    else if (filePath.endsWith(".css"))
        return "text/css";
    else if (filePath.endsWith(".js"))
        return "application/javascript";
    else if (filePath.endsWith(".json"))
        return "application/json";
    return "text/plain";
}