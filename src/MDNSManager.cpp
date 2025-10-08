#include "MDNSManager.h"
#include <ArduinoLog.h>

MDNSManager::MDNSManager(const char *hostname) : _hostname(hostname) {}

void MDNSManager::begin()
{
    if (!MDNS.begin(_hostname))
    {
        Log.infoln("[mDNS] Error setting up MDNS responder!");
    }
    else
    {
        Log.info("[mDNS] MDNS responder started with hostname: %s.local\n", _hostname);
        // Add service for web server (HTTP on TCP port 80)
        MDNS.addService("http", "tcp", 80);
        Log.infoln("[mDNS] HTTP service registered.");
    }
}

void MDNSManager::loop()
{
    MDNS.update(); // Keep the mDNS responder active
}