#include "OTAUpdater.h"
#include <ArduinoLog.h>

OTAUpdater::OTAUpdater() {}

void OTAUpdater::begin(const char *hostname)
{
    _hostname = hostname;
    ArduinoOTA.setPort(8266);
    ArduinoOTA.setHostname(_hostname);
    // ArduinoOTA.setPassword("admin");  // Uncomment and set password if needed

    ArduinoOTA.onStart([]()
                       { Log.infoln("[OTA] Start updating"); });

    ArduinoOTA.onEnd([]()
                     { Log.infoln("\n[OTA] Update complete"); });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          { Log.info("[OTA] Progress: %u%%\r", (progress / (total / 100))); });

    ArduinoOTA.onError([](ota_error_t error)
                       {
        Log.info("[OTA] Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
            Log.infoln("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
            Log.infoln("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
            Log.infoln("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
            Log.infoln("Receive Failed");
        else if (error == OTA_END_ERROR)
            Log.infoln("End Failed"); });
    ArduinoOTA.begin();
    Log.infoln("[OTA] Ready for updates");
}