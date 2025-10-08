#include "WifiConnector.h"
#include <ArduinoLog.h>

WiFiConnector::WiFiConnector(const char *ssid, const char *password, int statusLedPin)
    : _ssid(ssid), _password(password), _statusLedPin(statusLedPin)
{
    _currentState = WIFI_IDLE;
    _lastAttemptTimestamp = 0;
    _lastPulseTimestamp = 0;
    _ledPulseState = false;
    pinMode(_statusLedPin, OUTPUT);
    digitalWrite(_statusLedPin, HIGH); // LED is off (assuming common anode or inverted logic)
}

void WiFiConnector::connect()
{
    Log.infoln("[WiFi] Starting connection process...");
    WiFi.setSleepMode(WIFI_NONE_SLEEP); // Disable WiFi sleep mode
    _currentState = WIFI_CONNECTING;
    _lastAttemptTimestamp = millis();
    WiFi.begin(_ssid, _password);
}

void WiFiConnector::handleConnection()
{
    if (_currentState == WIFI_CONNECTING)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            Log.infoln("\n[WiFi] Connection successful!");
            Serial.print("[WiFi] IP Address: ");
            Log.infoln(WiFi.localIP());
            _currentState = WIFI_IDLE;
            digitalWrite(_statusLedPin, HIGH); // Turn LED off
        }
        else if (millis() - _lastAttemptTimestamp > CONNECTION_TIMEOUT)
        {
            Log.infoln("\n[WiFi] Connection failed. Will retry...");
            WiFi.disconnect();
            _currentState = WIFI_FAILED_WAITING;
            _lastAttemptTimestamp = millis();
        }
        else
        {
            // Still connecting, pulse the LED slowly (1 pulse every 2s)
            if (millis() - _lastPulseTimestamp > LED_PULSE_INTERVAL)
            {
                _lastPulseTimestamp = millis();
                _ledPulseState = !_ledPulseState;
                digitalWrite(_statusLedPin, _ledPulseState ? LOW : HIGH); // Pulse ON/OFF
            }
        }
    }
    else if (_currentState == WIFI_FAILED_WAITING)
    {
        // Solid ON during wait period
        digitalWrite(_statusLedPin, LOW);
        if (millis() - _lastAttemptTimestamp > RETRY_WAIT_PERIOD)
        {
            connect(); // Re-initiate connection process
        }
    }
    else if (_currentState == WIFI_IDLE && WiFi.status() != WL_CONNECTED)
    {
        Log.infoln("[WiFi] Connection lost. Attempting to reconnect...");
        connect();
    }
}

bool WiFiConnector::isConnected()
{
    return WiFi.status() == WL_CONNECTED;
}