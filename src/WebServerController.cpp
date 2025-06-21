#include "WebServerController.h"
#include "LittleFS.h"

WebServerController::WebServerController(int port, SettingsManager& settingsMgr, LedController& ledCtrl, Scheduler& scheduler)
    : _server(port), _settingsManager(settingsMgr), _ledController(ledCtrl), _scheduler(scheduler) {}

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
    DeviceSettings& settings = _settingsManager.getSettings();
    bool settingsChanged = false;

    if (_server.hasArg("state")) {
        settings.ledState = (_server.arg("state") == "true");
        settingsChanged = true;
    }
    if (_server.hasArg("brightness")) {
        settings.brightness = _server.arg("brightness").toInt();
        settingsChanged = true;
    }
    if (_server.hasArg("scheduleEnabled")) {
        settings.scheduleEnabled = (_server.arg("scheduleEnabled") == "true");
        settingsChanged = true;
    }
    if (_server.hasArg("startTime")) {
        settings.startTime = _server.arg("startTime");
        settingsChanged = true;
    }
    if (_server.hasArg("endTime")) {
        settings.endTime = _server.arg("endTime");
        settingsChanged = true;
    }

    if (settingsChanged) {
        _ledController.update(settings.ledState, settings.brightness);
        _scheduler.updateSchedule(settings.scheduleEnabled, settings.startTime, settings.endTime);
        _settingsManager.saveSettings();
    }

    _server.send(200, "application/json", "{\"success\":true}");
}

void WebServerController::handleStatus() {
    DeviceSettings& settings = _settingsManager.getSettings();
    String json = "{";
    json += "\"state\":" + String(settings.ledState? "true" : "false") + ",";
    json += "\"brightness\":" + String(settings.brightness) + ",";
    json += "\"sch_en\":" + String(settings.scheduleEnabled? "true" : "false") + ",";
    json += "\"sch_s\":\"" + settings.startTime + "\",";
    json += "\"sch_e\":\"" + settings.endTime + "\"";
    json += "}";
    _server.send(200, "application/json", json);
}

void WebServerController::handleNotFound() {
    _server.send(404, "text/plain", "404: Not Found");
}