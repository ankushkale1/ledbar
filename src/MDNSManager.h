#ifndef MDNS_MANAGER_H
#define MDNS_MANAGER_H

#include <ESP8266mDNS.h>
#include <Arduino.h> // For Log.infoln

class MDNSManager
{
public:
    MDNSManager();
    void begin(const char *hostname);
    void loop(); // New method to be called in main loop

private:
    const char *_hostname;
};

#endif