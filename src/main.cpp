#include <Arduino.h>
#include "SettingsManager.h"
#include "WifiConnector.h"
#include "LedController.h"
#include "TimeManager.h"
#include "Scheduler.h"
#include "WebServerController.h"

// --- Project Configuration ---
// WiFi Credentials
const char* WIFI_SSID = "JioFiber-4G";
const char* WIFI_PASSWORD = "Ak@00789101112";

// Pin Assignments
const int PWM_LED_PIN = D1;       // LED connected to D1 (GPIO5)
const int STATUS_LED_PIN = D4;    // On-board LED used for status (GPIO2)

// --- Global Object Instantiation ---
SettingsManager settingsManager;
LedController ledController(PWM_LED_PIN);
WiFiConnector wifiConnector(WIFI_SSID, WIFI_PASSWORD, STATUS_LED_PIN);
TimeManager timeManager;
Scheduler scheduler;
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
    ledController.update(settings.ledState, settings.brightness);

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

    // 6. Initialize and start the Web Server
    webServerController.begin();

    Serial.println("[Main] Setup complete. System running.");
}

void loop() {
    // Must be called every loop to service web requests
    webServerController.handleClient();

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
            
            SchedulerAction action = scheduler.checkSchedule(
                timeManager.getHours(),
                timeManager.getMinutes(),
                settings.ledState
            );

            if (action!= NO_ACTION) {
                Serial.print("[Main] Scheduler triggered action: ");
                if (action == TURN_ON) {
                    Serial.println("TURN_ON");
                    settings.ledState = true;
                } else { // TURN_OFF
                    Serial.println("TURN_OFF");
                    settings.ledState = false;
                }
                
                // Apply the new state and save it
                ledController.update(settings.ledState, settings.brightness);
                settingsManager.saveSettings();
            }
        }
    }
}