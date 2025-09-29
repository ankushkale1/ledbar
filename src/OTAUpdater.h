#ifndef OTAUPDATER_H
#define OTAUPDATER_H

#include <Arduino.h>
#include <ArduinoOTA.h>

class OTAUpdater
{
public:
    OTAUpdater(const char *hostname);
    void begin();

private:
    const char *_hostname;
};

#endif