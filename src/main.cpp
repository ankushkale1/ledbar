#include <Arduino.h>
#include <ArduinoLog.h>
#include "SettingsManager.h"
#include "WifiConnector.h"
#include "LedController.h"
#include "TimeManager.h"
#include "Scheduler.h"
#include "MDNSManager.h"
#include "WebServerController.h"
#include "MotionSensor.h"
#include "OTAUpdater.h"
#include "WebsocketLogger.h"
#include "IrManager.h"
#include <WiFi.h>
#include "esp_bt.h"      // For Bluetooth controller functions
#include "esp_bt_main.h" // For esp_bt_controller_deinit

// --- Project Configuration ---
// WiFi Credentials
// Wifi not working on ESP32-C3 board due to hardware component placement issues
// const char *WIFI_SSID = "JioFiber-4G";
// const char *WIFI_PASSWORD = "Ak@00789101112";

// ESP32-C3 Pin Configuration Notes
// The ESP32-C3 has a different pinout compared to other ESP32 variants. When
// selecting GPIO pins for peripherals like LEDs and IR receivers, it's important
// to avoid using pins that are reserved for special functions such as
// boot-mode (strapping) pins and the default serial pins.

// 6 PWM Channels: Use GPIO 3, GPIO 4, GPIO 5, GPIO 6, GPIO 7, and GPIO 10. These pins are all general-purpose and support PWM via the LEDC peripheral.

// 1 IR Receiver Pin: Use GPIO 1. (GPIO 0 is also an option, but it's often connected to the onboard "BOOT" button, so GPIO 1 is a safer choice).

// Pins to Avoid
// For stability, you should avoid the following pins:

// GPIO 9, GPIO 8, GPIO 2: These are strapping pins. They are checked at boot-up to determine the boot mode. Connecting circuits to them can prevent your board from starting or put it in the wrong mode.

// GPIO 20 (RX) & GPIO 21 (TX): These are the default UART pins. You need them to upload your code and see serial monitor output. Using them for other purposes will block your ability to program the board.

// Pin Assignments
// const int STATUS_LED_PIN = 8; // On-board LED for ESP32-C3 (GPIO8)
const int INVERTING_LOGIC = true;
const int IR_RECEIVER_PIN = 1; // Using GPIO10 for IR receiver on ESP32-C3
// const int MOTION_ON_HOUR = 21;
// const int MOTION_OFF_HOUR = 6;

// --- Global Object Instantiation ---
SettingsManager settingsManager;
// const char *MDNS_HOSTNAME = settingsManager.getSettings().mDNSName.c_str(); // mDNS hostname for the device
LedController ledController(INVERTING_LOGIC); // true for inverted logic (active-low LEDs)
// WiFiConnector wifiConnector(WIFI_SSID, WIFI_PASSWORD, STATUS_LED_PIN);
// TimeManager timeManager;
// Scheduler scheduler;
// MDNSManager mdnsManager(MDNS_HOSTNAME);
// WebSocketsServer webSocket(81);
// WebsocketLogger websocketLogger(webSocket);
// WebServerController webServerController(80, webSocket, settingsManager, ledController, scheduler, timeManager);
// MotionSensor motionSensor(MOTION_SENSOR_PIN);
// OTAUpdater otaUpdater(MDNS_HOSTNAME);
IrManager irManager(IR_RECEIVER_PIN);

// --- Timer for non-blocking scheduler check ---
unsigned long lastSchedulerCheck = 0;
const long SCHEDULER_CHECK_INTERVAL = 1000; // Check every second

void disable_wifi_bluetooth()
{
    // --- 1. Disable Wi-Fi ---
    // This is the standard Arduino way to turn off the Wi-Fi radio
    WiFi.mode(WIFI_OFF);
    Log.infoln("Wi-Fi radio is OFF.");

    // --- 2. Disable Bluetooth ---
    // This uses the low-level functions to completely shut down the BT controller

    // Disable the Bluetooth controller
    if (esp_bt_controller_disable() != ESP_OK)
    {
        Log.infoln("Failed to disable BT controller.");
    }

    // De-initialize the Bluetooth controller
    if (esp_bt_controller_deinit() != ESP_OK)
    {
        Log.infoln("Failed to de-initialize BT controller.");
    }

    // Release the memory used by the Bluetooth stack
    // For the ESP32-C3 (which is BLE only), we use ESP_BT_MODE_BLE
    if (esp_bt_mem_release(ESP_BT_MODE_BLE) != ESP_OK)
    {
        Log.infoln("Failed to release BT memory.");
    }

    Log.infoln("Bluetooth is fully OFF and memory is released.");
}

void setup()
{
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial);
    Log.infoln("\n[Main] Booting device...");

    delay(1000); // Give some time for Serial to initialize

    disable_wifi_bluetooth();

    // Scan for WiFi networks
    // Log.infoln("[WiFi] Scanning for networks...");
    // WiFi.mode(WIFI_STA);
    // int n = WiFi.scanNetworks();
    // if (n == 0)
    // {
    //     Log.infoln("[WiFi] No networks found.");
    // }
    // else
    // {
    //     Log.infoln("[WiFi] Found %d networks:", n);
    //     for (int i = 0; i < n; ++i)
    //     {
    //         Log.infoln("[WiFi]  %d: %s (%d)", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
    //     }
    // }

    // 1. Initialize filesystem and load settings
    settingsManager.begin();
    DeviceSettings &settings = settingsManager.getSettings();

    // 2. Initialize LED controller and apply loaded settings
    ledController.begin();
    ledController.update(settings);

    // 3. Initialize Scheduler with loaded settings
    // scheduler.updateSchedule(settings);

    // 3.5. Initialize Motion Sensor
    // motionSensor.begin();

    // 3.6. Initialize IR Manager
    irManager.begin();

    // 4. Connect to WiFi (this is a blocking section by design for initial setup)
    // wifiConnector.connect();
    // while (!wifiConnector.isConnected())
    // {
    //     wifiConnector.handleConnection(); // Manages status LED and retries
    //     // Yield to allow background processes to run
    //     yield();
    // }

    // 5. Initialize Time Manager and perform initial sync
    // timeManager.begin();
    // timeManager.setTimezone(settings.gmtOffsetSeconds);
    // timeManager.update(); // Force initial update

    // 6. Initialize mDNS
    // mdnsManager.begin();

    // 7. Initialize and start the Web Server
    // webServerController.begin();
    // websocketLogger.begin();

    // otaUpdater.begin();
    // Log.infoln("[OTA] Ready for updates");

    Log.infoln("[Main] Setup complete. System running.");
}

void loop()
{
    // unsigned long loopStartTime = millis();

    // Handle OTA updates
    ArduinoOTA.handle();

    // Must be called every loop to service web requests
    // webServerController.handleClient();
    // websocketLogger.loop();

    // Manages WiFi connection state (e.g., handles reconnects)
    // wifiConnector.handleConnection();

    // Handle IR remote
    irManager.loop();
    if (irManager.available())
    {
        uint64_t irCode = irManager.read();
        String irCodeHex = String(irCode, HEX);
        irCodeHex.toUpperCase();
        Log.infoln("[Main] IR Code Received: %s", irCodeHex.c_str());
        // String payload = "ir_code:" + irCodeHex;
        //  webSocket.broadcastTXT(payload);

        DeviceSettings &settings = settingsManager.getSettings();

        Log.infoln("[Main] IR Code Received: %s , %s %s", irCodeHex.c_str(), settings.irCodeBrightnessUp.c_str(), settings.irCodeBrightnessDown.c_str());
        if (irCodeHex == settings.irCodeBrightnessUp)
        {
            Log.infoln("[Main] Brightness Up");
            for (auto &channel : settings.channels)
            {
                if (channel.state)
                {
                    channel.brightness = min(100, channel.brightness + 10);
                }
            }
            ledController.update(settings);
            settingsManager.saveSettings();
        }
        else if (irCodeHex == settings.irCodeBrightnessDown)
        {
            Log.infoln("[Main] Brightness Down");
            for (auto &channel : settings.channels)
            {
                if (channel.state)
                {
                    channel.brightness = max(0, channel.brightness - 10);
                }
            }
            ledController.update(settings);
            settingsManager.saveSettings();
        }
        else
        {
            for (auto &channel : settings.channels)
            {
                if (channel.irCode == irCodeHex)
                {
                    Log.infoln("[Main] Toggling channel %s is now %s", channel.channelName.c_str(), channel.state ? "ON" : "OFF");
                    channel.state = !channel.state;
                    ledController.update(settings);
                    settingsManager.saveSettings();
                    break; // Assuming one IR code per channel
                }
            }
        }
    }

    // Periodically update time from NTP server
    // if (wifiConnector.isConnected())
    // {
    //     timeManager.update();
    // }

    // Motion detection logic
    // int currentHour = timeManager.getHours();
    // if (motionSensor.motionDetected() && (currentHour >= MOTION_ON_HOUR || currentHour < MOTION_OFF_HOUR))
    // {
    //     Log.infoln("[Main] Motion detected at night. Turning on lights.");
    //     DeviceSettings &settings = settingsManager.getSettings();

    //     bool settingsChanged = false;
    //     for (auto &channel : settings.channels)
    //     {
    //         if (!channel.state)
    //         {
    //             channel.state = true;
    //             settingsChanged = true;
    //         }
    //     }
    //     if (settingsChanged)
    //     {
    //         ledController.update(settings);
    //     }

    //     // Keep the lights on for 5 minutes
    //     delay(300000); // 5 minutes delay

    //     Log.infoln("[Main] Motion timeout. Turning off lights.");
    //     settingsChanged = false;
    //     for (auto &channel : settings.channels)
    //     {
    //         if (channel.state)
    //         {
    //             channel.state = false;
    //             settingsChanged = true;
    //         }
    //     }
    //     if (settingsChanged)
    //     {
    //         ledController.update(settings);
    //     }
    // }
    // else
    // {
    // Check the scheduler periodically (non-blocking)
    // if (millis() - lastSchedulerCheck > SCHEDULER_CHECK_INTERVAL)
    // {
    //     lastSchedulerCheck = millis();

    //     if (wifiConnector.isConnected())
    //     {
    //         DeviceSettings &settings = settingsManager.getSettings();
    //         std::vector<SchedulerAction> actions = scheduler.checkSchedule(
    //             timeManager.getHours(),
    //             timeManager.getMinutes());

    //         bool settingsChanged = false;

    //         for (const auto &action : actions)
    //         {
    //             Log.infoln("[Main] Scheduler Action: Channel: %s, State: %s, Brightness: %d\n",
    //                        action.channel.c_str(),
    //                        action.stateOnOFF ? "ON" : "OFF",
    //                        action.brightness);
    //             // Update the LED controller based on the action

    //             for (auto &channel : settings.channels)
    //             {
    //                 if (channel.pin == action.channel)
    //                 {
    //                     if (channel.scheduleEnabled && action.stateOnOFF)
    //                         channel.schedulerActive = true;
    //                     else
    //                         channel.schedulerActive = false;
    //                     settingsChanged = true;
    //                 }
    //             }
    //             if (settingsChanged)
    //             {
    //                 ledController.update(settings);
    //             }
    //         }
    //     }
    // }
    //}

    // Log.verboseln("[Main] Loop duration: %lu ms", millis() - loopStartTime);
}