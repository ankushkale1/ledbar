#ifndef WEBSERVER_CONTROLLER_H
#define WEBSERVER_CONTROLLER_H

#include <ESP8266WebServer.h>
#include "SettingsManager.h"
#include "LedController.h"
#include "Scheduler.h"

class WebServerController {
public:
    WebServerController(int port, SettingsManager& settingsMgr, LedController& ledCtrl, Scheduler& scheduler);
    void begin();
    void handleClient();

private:
    ESP8266WebServer _server;
    SettingsManager& _settingsManager;
    LedController& _ledController;
    Scheduler& _scheduler;

    void handleRoot();
    void handleSettings();
    void handleStatus();
    void handleNotFound();
};

#endif