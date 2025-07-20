#ifndef WEBSERVER_CONTROLLER_H
#define WEBSERVER_CONTROLLER_H

#include <ESP8266WebServer.h>
#include "SettingsManager.h"
#include "LedController.h"
#include "Scheduler.h"
#include "TimeManager.h"

class WebServerController {
public:
    WebServerController(int port, SettingsManager& settingsMgr, LedController& ledCtrl, Scheduler& scheduler, TimeManager& timeMgr);
    void begin();
    void handleClient();
    void serveFile(const String& filePath);
    String getContentType(const String& filePath);

private:
    ESP8266WebServer _server;
    SettingsManager& _settingsManager;
    LedController& _ledController;
    Scheduler& _scheduler;
    TimeManager& _timeManager;

    void handleRoot();
    void handleSettings();
    void handleStatus();
    void handleNotFound();
};

#endif