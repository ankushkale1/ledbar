#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <NTPClient.h>
#include <WiFiUdp.h>

class TimeManager {
public:
    TimeManager();
    void begin();
    void update();
    String getFormattedTime();
    int getHours();
    int getMinutes();

private:
    WiFiUDP _ntpUDP;
    NTPClient _timeClient;
    unsigned long _lastNtpUpdateTime;
    const long NTP_UPDATE_INTERVAL = 3600000; // 1 hour in ms
};

#endif