#include "TimeManager.h"

// GMT offset is 0, we handle timezone logic if needed elsewhere or assume UTC
TimeManager::TimeManager() : _timeClient(_ntpUDP, "pool.ntp.org", 0, 60000) {
    _lastNtpUpdateTime = 0;
}

void TimeManager::begin() {
    _timeClient.begin();
    Serial.println(" TimeManager initialized.");
}

void TimeManager::update() {
    // Only update from NTP server periodically to reduce network traffic
    if (millis() - _lastNtpUpdateTime > NTP_UPDATE_INTERVAL || _lastNtpUpdateTime == 0) {
        if (_timeClient.forceUpdate()) {
            _lastNtpUpdateTime = millis();
            Serial.print(" NTP time updated: ");
            Serial.println(getFormattedTime());
        } else {
            Serial.println(" Failed to update NTP time.");
        }
    }
}

String TimeManager::getFormattedTime() {
    return _timeClient.getFormattedTime();
}

int TimeManager::getHours() {
    return _timeClient.getHours();
}

int TimeManager::getMinutes() {
    return _timeClient.getMinutes();
}