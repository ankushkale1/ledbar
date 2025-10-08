#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <NTPClient.h>
#include <WiFiUdp.h>

#define NTP_UPDATE_INTERVAL 3600000 // 1 hour in ms

class TimeManager
{
public:
    TimeManager();
    void begin();
    void update();
    void setTimezone(long gmtOffsetSeconds);
    String getFormattedTime();
    int getHours();
    int getMinutes();

private:
    WiFiUDP _ntpUDP;
    NTPClient _timeClient;
    long _current_gmtOffsetSeconds;
};

#endif