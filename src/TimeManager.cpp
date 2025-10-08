#include "TimeManager.h"
#include <ArduinoLog.h>

// Initialize with a default offset of 0. It will be updated from settings.
TimeManager::TimeManager() : _timeClient(_ntpUDP, "pool.ntp.org", 0, NTP_UPDATE_INTERVAL)
{
}

void TimeManager::begin()
{
    _timeClient.begin();
    Log.infoln("[TimeMgr] Initialized for IST (UTC+5:30).");
}

void TimeManager::update()
{
    if (_timeClient.update())
    {
        Log.infoln("[TimeMgr] NTP time updated: %s", getFormattedTime().c_str());
    }
}

void TimeManager::setTimezone(long gmtOffsetSeconds)
{
    _timeClient.setTimeOffset(gmtOffsetSeconds);
    // Force an update to apply the new timezone immediately
    _timeClient.forceUpdate();
}

String TimeManager::getFormattedTime()
{
    return _timeClient.getFormattedTime();
}

int TimeManager::getHours()
{
    return _timeClient.getHours();
}

int TimeManager::getMinutes()
{
    return _timeClient.getMinutes();
}