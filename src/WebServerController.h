#ifndef WEBSERVER_CONTROLLER_H
#define WEBSERVER_CONTROLLER_H

#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include "SettingsManager.h"
#include "LedController.h"
#include "Scheduler.h"
#include "TimeManager.h"

class WebServerController
{
public:
    WebServerController(int port, WebSocketsServer &ws, SettingsManager &settingsMgr, LedController &ledCtrl, Scheduler &scheduler, TimeManager &timeMgr);
    void begin();
    void handleClient();
    void serveFile(const String &filePath);
    String getContentType(const String &filePath);

private:
    ESP8266WebServer _server;
    File fsUploadFile;
    String _uploadFilename;
    WebSocketsServer &_ws;
    SettingsManager &_settingsManager;
    LedController &_ledController;
    Scheduler &_scheduler;
    TimeManager &_timeManager;

    unsigned long _lastHeapTime = 0;

    void handleRoot();
    void handleSettings();
    void handleStatus();
    void handleVersion();
    void handleNotFound();
    void handleDownloadSettings();
    void handleFileUpload();
    void handleUpload();
    void handleRestart();
    void handleHeap();
    void broadcastHeap();
};

#endif