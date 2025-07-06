#include "TimeManager.h"

// Initialize with a default offset of 0. It will be updated from settings.
TimeManager::TimeManager() : _timeClient(_ntpUDP, "pool.ntp.org", 0, 60000) {
    _lastNtpUpdateTime = 0;
}

void TimeManager::begin() {
    _timeClient.begin();
    Serial.println("[TimeMgr] Initialized for IST (UTC+5:30).");
}

void TimeManager::update() {
    // Only update from NTP server periodically to reduce network traffic
    if (millis() - _lastNtpUpdateTime > NTP_UPDATE_INTERVAL || _lastNtpUpdateTime == 0) {
        if (_timeClient.forceUpdate()) {
            _lastNtpUpdateTime = millis();
            Serial.print("[TimeMgr] NTP time updated: ");
            Serial.println(getFormattedTime());
        } else {
            Serial.println("[TimeMgr] Failed to update NTP time.");
        }
    }
}

void TimeManager::setTimezone(long gmtOffsetSeconds) {
    _timeClient.setTimeOffset(gmtOffsetSeconds);
    // Force an update to apply the new timezone immediately
    if (_timeClient.forceUpdate()) {
        _lastNtpUpdateTime = millis();
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