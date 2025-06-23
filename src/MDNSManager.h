#ifndef MDNS_MANAGER_H
#define MDNS_MANAGER_H

#include <ESP8266mDNS.h>
#include <Arduino.h> // For Serial.println

class MDNSManager {
public:
    MDNSManager(const char* hostname);
    void begin();
    void loop(); // New method to be called in main loop

private:
    const char* _hostname;
};

#endif