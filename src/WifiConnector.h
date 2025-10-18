#ifndef WIFI_CONNECTOR_H
#define WIFI_CONNECTOR_H

#include <ESP8266WiFi.h>

class WiFiConnector {
public:
    WiFiConnector(const char* ssid, const char* password, int statusLedPin = -1);
    void connect();
    void handleConnection();
    bool isConnected();

private:
    const char* _ssid;
    const char* _password;
    int _statusLedPin;

    enum WiFiState {
        WIFI_IDLE,
        WIFI_CONNECTING,
        WIFI_FAILED_WAITING
    };
    WiFiState _currentState;

    unsigned long _lastAttemptTimestamp;
    unsigned long _lastPulseTimestamp;
    bool _ledPulseState;

    const unsigned long CONNECTION_TIMEOUT = 30000; // 30 seconds
    const unsigned long RETRY_WAIT_PERIOD = 5000;   // 5 seconds
    const unsigned long LED_PULSE_INTERVAL = 2000;  // 2 seconds
};

#endif