#ifndef MDNS_MANAGER_H
#define MDNS_MANAGER_H

#include <ESPmDNS.h>
#include <Arduino.h> // For Log.infoln

class MDNSManager
{
public:
    MDNSManager(const char *hostname);
    void begin();

private:
    const char *_hostname;
};

#endif