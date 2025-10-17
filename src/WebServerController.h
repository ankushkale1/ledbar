#ifndef WEBSERVER_CONTROLLER_H
#define WEBSERVER_CONTROLLER_H

#include <WebServer.h>
#include <WebSocketsServer.h>
#include "SettingsManager.h"
#include "LedController.h"
#include "Scheduler.h"
#include "TimeManager.h"

class WebServerController {
public:
    WebServerController(int port, WebSocketsServer& ws, SettingsManager& settingsMgr, LedController& ledCtrl, Scheduler& scheduler, TimeManager& timeMgr);
    void begin();
    void handleClient();
    void serveFile(const String& filePath);
    String getContentType(const String& filePath);

private:
    WebServer _server;
    WebSocketsServer& _ws;
    SettingsManager& _settingsManager;
    LedController& _ledController;
    Scheduler& _scheduler;
    TimeManager& _timeManager;

    void handleRoot();
    void handleSettings();
    void handleStatus();
    void handleVersion();
    void handleNotFound();
};

#endif