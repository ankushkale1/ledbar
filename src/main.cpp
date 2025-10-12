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

// DONT USE PINS
// D4	GPIO2	Boot Mode Pin & LED. Connected to the onboard LED. Must be floating or pulled HIGH during boot.
// D8	GPIO15	Boot Mode Pin. Must be pulled LOW for the board to boot normally. Connecting a component that pulls it HIGH will prevent the board from starting.

// SAFE PINS
//  D0 (GPIO16)	- NO PWM, Can be used for output. Cannot be used for interrupts, PWM, or I2C. Used to wake the ESP from deep sleep.
//  D1 (GPIO5)  - PWM, Often used for I2C (SCL)
//  D2 (GPIO4)  - PWM, Often used for I2C (SDA)
//  D3	GPIO0	- PWM, Boot Mode Pin. Already internally pulled high, but circuit must make sure it stays high at start, my 2.2k ohm resistor
//                connected between bjt gate and esp pin is enough to make sure pin stays high at start. Connected to the "FLASH" button.
//                The ESP8266 will enter programming mode if this pin is pulled LOW on boot.
//                Your circuit must ensure this pin is HIGH when the board powers up.
//  D5 (GPIO14) - PWM, Often used for SPI (SCK)
//  D6 (GPIO12) - PWM, Often used for SPI (MISO)
//  D7 (GPIO13) - PWM, Often used for SPI (MOSI)

// CAN USE BUT NOT RECOMMENDED
//  TX	GPIO1	Used for the hardware Serial Port (UART0). It is essential for uploading code and using the Serial Monitor for debugging.
//  RX	GPIO3	Used for the hardware Serial Port (UART0). Essential for receiving serial data.
//  A0	ADC0	This is the only analog input pin on the board. While it can be used as a digital pin, you should save it for reading analog sensors (like potentiometers, LDRs, etc.).
//  SD2, SD3	GPIO9, 10	These are typically connected internally to the ESP8266's flash memory chip and are not exposed or safe to use on most NodeMCU boards.

// --- Project Configuration ---
// WiFi Credentials
const char *WIFI_SSID = "JioFiber-4G";
const char *WIFI_PASSWORD = "Ak@00789101112";

// Pin Assignments
const int STATUS_LED_PIN = D4; // On-board LED used for status (GPIO2)
const int INVERTING_LOGIC = true;
// const int MOTION_SENSOR_PIN = D0; // we dont have enough safe pins so disabling this
const int IR_RECEIVER_PIN = D4;
const int MOTION_ON_HOUR = 21;
const int MOTION_OFF_HOUR = 6;

// --- Global Object Instantiation ---
SettingsManager settingsManager;
const char *MDNS_HOSTNAME = settingsManager.getSettings().mDNSName.c_str(); // mDNS hostname for the device
LedController ledController(INVERTING_LOGIC);                               // true for inverted logic (active-low LEDs)
WiFiConnector wifiConnector(WIFI_SSID, WIFI_PASSWORD, STATUS_LED_PIN);
TimeManager timeManager;
Scheduler scheduler;
MDNSManager mdnsManager(MDNS_HOSTNAME);
WebSocketsServer webSocket(81);
WebsocketLogger websocketLogger(webSocket);
WebServerController webServerController(80, webSocket, settingsManager, ledController, scheduler, timeManager);
// MotionSensor motionSensor(MOTION_SENSOR_PIN);
OTAUpdater otaUpdater(MDNS_HOSTNAME);
IrManager irManager(IR_RECEIVER_PIN);

// --- Timer for non-blocking scheduler check ---
unsigned long lastSchedulerCheck = 0;
const long SCHEDULER_CHECK_INTERVAL = 1000; // Check every second

void setup()
{
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_VERBOSE, &websocketLogger);
    Log.infoln("\n[Main] Booting device...");

    // 1. Initialize filesystem and load settings
    // settingsManager.begin();
    DeviceSettings &settings = settingsManager.getSettings();

    // 2. Initialize LED controller and apply loaded settings
    ledController.begin();
    ledController.update(settings);

    // 3. Initialize Scheduler with loaded settings
    scheduler.updateSchedule(settings);

    // 3.5. Initialize Motion Sensor
    // motionSensor.begin();

    // 3.6. Initialize IR Manager
    irManager.begin();

    // 4. Connect to WiFi (this is a blocking section by design for initial setup)
    wifiConnector.connect();
    while (!wifiConnector.isConnected())
    {
        wifiConnector.handleConnection(); // Manages status LED and retries
        // Yield to allow background processes to run
        yield();
    }

    // 5. Initialize Time Manager and perform initial sync
    timeManager.begin();
    timeManager.setTimezone(settings.gmtOffsetSeconds);
    timeManager.update(); // Force initial update

    // 6. Initialize mDNS
    mdnsManager.begin();

    // 7. Initialize and start the Web Server
    webServerController.begin();
    websocketLogger.begin();

    otaUpdater.begin();
    Log.infoln("[OTA] Ready for updates");

    Log.infoln("[Main] Setup complete. System running.");
}

void loop()
{
    // unsigned long loopStartTime = millis();

    // Handle OTA updates
    ArduinoOTA.handle();

    // Must be called every loop to service web requests
    webServerController.handleClient();
    websocketLogger.loop();

    // Keep mDNS service active
    mdnsManager.loop();

    // Manages WiFi connection state (e.g., handles reconnects)
    wifiConnector.handleConnection();

    // Handle IR remote
    irManager.loop();
    if (irManager.available())
    {
        uint64_t irCode = irManager.read();
        String irCodeHex = String(irCode, HEX);
        irCodeHex.toUpperCase();
        Log.infoln("[Main] IR Code Received: %s", irCodeHex.c_str());
        String payload = "ir_code:" + irCodeHex;
        webSocket.broadcastTXT(payload);

        DeviceSettings &settings = settingsManager.getSettings();

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
                    Log.infoln("[Main] Toggling channel %s", channel.channelName.c_str());
                    channel.state = !channel.state;
                    ledController.update(settings);
                    settingsManager.saveSettings();
                    break; // Assuming one IR code per channel
                }
            }
        }
    }

    // Periodically update time from NTP server
    if (wifiConnector.isConnected())
    {
        timeManager.update();
    }

    // Motion detection logic
    int currentHour = timeManager.getHours();
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
    if (millis() - lastSchedulerCheck > SCHEDULER_CHECK_INTERVAL)
    {
        lastSchedulerCheck = millis();

        if (wifiConnector.isConnected())
        {
            DeviceSettings &settings = settingsManager.getSettings();
            std::vector<SchedulerAction> actions = scheduler.checkSchedule(
                timeManager.getHours(),
                timeManager.getMinutes());

            bool settingsChanged = false;

            for (const auto &action : actions)
            {
                Log.infoln("[Main] Scheduler Action: Channel: %s, State: %s, Brightness: %d\n",
                           action.channel.c_str(),
                           action.stateOnOFF ? "ON" : "OFF",
                           action.brightness);
                // Update the LED controller based on the action

                for (auto &channel : settings.channels)
                {
                    if (channel.pin == action.channel)
                    {
                        if (channel.scheduleEnabled && action.stateOnOFF)
                            channel.schedulerActive = true;
                        else
                            channel.schedulerActive = false;
                        settingsChanged = true;
                    }
                }
                if (settingsChanged)
                {
                    ledController.update(settings);
                }
            }
        }
    }
    //}

    // Log.verboseln("[Main] Loop duration: %lu ms", millis() - loopStartTime);
}