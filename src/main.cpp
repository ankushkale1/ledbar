#include <Arduino.h>
#include "SettingsManager.h"
#include "WifiConnector.h"
#include "LedController.h"
#include "TimeManager.h"
#include "Scheduler.h"
#include "MDNSManager.h"
#include "WebServerController.h"

// --- Project Configuration ---
// WiFi Credentials
const char* WIFI_SSID = "JioFiber-4G";
const char* WIFI_PASSWORD = "Ak@00789101112";

// Pin Assignments
const int STATUS_LED_PIN = D4;    // On-board LED used for status (GPIO2)
const char* MDNS_HOSTNAME = "ledbar"; // mDNS hostname for the device

// --- Global Object Instantiation ---
SettingsManager settingsManager;
LedController ledController(true); // true for inverted logic (active-low LEDs)
WiFiConnector wifiConnector(WIFI_SSID, WIFI_PASSWORD, STATUS_LED_PIN);
TimeManager timeManager;
Scheduler scheduler;
MDNSManager mdnsManager(MDNS_HOSTNAME);
WebServerController webServerController(80, settingsManager, ledController, scheduler);

// --- Timer for non-blocking scheduler check ---
unsigned long lastSchedulerCheck = 0;
const long SCHEDULER_CHECK_INTERVAL = 1000; // Check every second

void setup() {
    Serial.begin(115200);
    Serial.println("\n[Main] Booting device...");

    // 1. Initialize filesystem and load settings
    settingsManager.begin();
    DeviceSettings& settings = settingsManager.getSettings();

    // 2. Initialize LED controller and apply loaded settings
    ledController.begin();
    ledController.update(settings);

    // 3. Initialize Scheduler with loaded settings
    scheduler.updateSchedule(settings.scheduleEnabled, settings.startTime, settings.endTime);
    
    // 4. Connect to WiFi (this is a blocking section by design for initial setup)
    wifiConnector.connect();
    while (!wifiConnector.isConnected()) {
        wifiConnector.handleConnection(); // Manages status LED and retries
        // Yield to allow background processes to run
        yield(); 
    }

    // 5. Initialize Time Manager and perform initial sync
    timeManager.begin();
    timeManager.update(); // Force initial update
    
    // 6. Initialize mDNS
    mdnsManager.begin();

    // 7. Initialize and start the Web Server
    webServerController.begin();

    Serial.println("[Main] Setup complete. System running.");
}

void loop() {
    // Must be called every loop to service web requests
    webServerController.handleClient();

    // Keep mDNS service active
    mdnsManager.loop();

    // Manages WiFi connection state (e.g., handles reconnects)
    wifiConnector.handleConnection();

    // Periodically update time from NTP server
    if (wifiConnector.isConnected()) {
        timeManager.update();
    }

    // Check the scheduler periodically (non-blocking)
    if (millis() - lastSchedulerCheck > SCHEDULER_CHECK_INTERVAL) {
        lastSchedulerCheck = millis();

        if (wifiConnector.isConnected()) {
            DeviceSettings& settings = settingsManager.getSettings();

            // Check if any channel is on to pass to the scheduler
            bool anyChannelOn = false;
            for (const auto& channel : settings.channels) {
                if (channel.state) {
                    anyChannelOn = true;
                    break;
                }
            }

            SchedulerAction action = scheduler.checkSchedule(
                timeManager.getHours(),
                timeManager.getMinutes(),
                anyChannelOn
            );

            if (action!= NO_ACTION) {
                bool newState = (action == TURN_ON);
                Serial.print("[Main] Scheduler triggered action: ");
                Serial.println(newState ? "TURN_ON" : "TURN_OFF");
                for (auto& channel : settings.channels) {
                    channel.state = newState;
                }
                ledController.update(settings);
                settingsManager.saveSettings();
            }
        }
    }
}