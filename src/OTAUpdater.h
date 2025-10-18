#ifndef OTAUPDATER_H
#define OTAUPDATER_H

#include <Arduino.h>
#include <ArduinoOTA.h>

class OTAUpdater
{
public:
    OTAUpdater();
    void begin(const char *hostname);

private:
    const char *_hostname;
};

#endif