/*
 * LedBar Controller - Main Application File
 * Architecture: Fully Asynchronous with ESPAsync_WiFiManager and LittleFS
 * UPDATED: Corrected compilation errors and refactored for ESPAsync_WiFiManager
 */

// Core ESP8266 & WiFi Libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>

// Asynchronous Web Server and related libraries
#include <ESPAsyncWebServer.h>

// Asynchronous WiFi Manager by khoih-prog
// The .h file provides the class definitions.
#include <ESPAsync_WiFiManager.h>
// The -Impl.h file provides the implementation. Including it here in the
// main source file prevents multiple-definition linker errors in larger projects.
#include <ESPAsync_WiFiManager-Impl.h>

// Filesystem Library - Using LittleFS
#include <LittleFS.h>

// Other project libraries
#include <ArduinoJson.h>
#include <TimeAlarms.h>

// -- Hardware Pins --
const int PWM_OUTPUT_PIN = D1; // GPIO5 on NodeMCU
const int CONFIG_RESET_PIN = D2; // GPIO4. Short this pin to GND to trigger config portal.

// -- LED PWM Settings (for inverting driver) --
const int PWM_RANGE = 255; // Using 8-bit PWM

// -- Configuration Struct --
struct Config {
    // FIX: Restored char arrays to their correct sizes.
    char deviceName[33] = "ledbar";
    int brightness = 255;
    bool state = true; // true = ON, false = OFF
    bool timerEnabled = false;
    char onTime[6] = "07:00"; // HH:MM format
    char offTime[6] = "23:00"; // HH:MM format
};

Config config; // Global config object

// -- Global Objects --
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
// FIX: Instantiate AsyncDNSServer and the new ESPAsync_WiFiManager
AsyncDNSServer dns;
ESPAsync_WiFiManager wifiManager(&server, &dns);

bool shouldSaveConfig = false;

// Variables to hold the alarm IDs
AlarmID_t onTimerId = dtINVALID_ALARM_ID;
AlarmID_t offTimerId = dtINVALID_ALARM_ID;


// -- Function Declarations --
void setLed(bool newState, int newBrightness);
void saveConfiguration(const char *filename, const Config &conf);
void loadConfiguration(const char *filename, Config &conf);
void notifyClients();
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void setupTime();
void digitalClockDisplay();
void triggerLedOn();
void triggerLedOff();
void setupTimers();


// =================================================================
// Configuration Management (JSON with LittleFS)
// =================================================================

void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void saveConfiguration(const char *filename, const Config &conf) {
    LittleFS.remove(filename);
    File file = LittleFS.open(filename, "w");
    if (!file) {
        Serial.println("Failed to create config file for writing");
        return;
    }

    StaticJsonDocument<256> doc; // Reduced size, 1024 is overkill for this data

    doc["deviceName"] = conf.deviceName;
    doc["brightness"] = conf.brightness;
    doc["state"] = conf.state;
    doc["timerEnabled"] = conf.timerEnabled;
    doc["onTime"] = conf.onTime;
    doc["offTime"] = conf.offTime;

    if (serializeJson(doc, file) == 0) {
        Serial.println("Failed to write to config file");
    } else {
        Serial.println("Configuration saved");
    }
    file.close();
}

void loadConfiguration(const char *filename, Config &conf) {
    if (LittleFS.exists(filename)) {
        File file = LittleFS.open(filename, "r");
        
        StaticJsonDocument<256> doc;

        DeserializationError error = deserializeJson(doc, file);
        if (error) {
            Serial.println("Failed to read config file, using default configuration");
            return;
        }

        // FIX: The original logic here was correct.
        strlcpy(conf.deviceName, doc["deviceName"] | "ledbar", sizeof(conf.deviceName));
        conf.brightness = doc["brightness"] | 255;
        conf.state = doc["state"] | true;
        conf.timerEnabled = doc["timerEnabled"] | false;
        strlcpy(conf.onTime, doc["onTime"] | "07:00", sizeof(conf.onTime));
        strlcpy(conf.offTime, doc["offTime"] | "23:00", sizeof(conf.offTime));

        file.close();
        Serial.println("Configuration loaded");
    } else {
        Serial.println("Config file not found, using default configuration");
    }
}


// =================================================================
// WebSocket and LED Control
// =================================================================

void notifyClients() {
    StaticJsonDocument<256> doc;
    doc["state"] = config.state;
    doc["brightness"] = config.brightness;
    doc["timerEnabled"] = config.timerEnabled;
    doc["onTime"] = config.onTime;
    doc["offTime"] = config.offTime;
    
    // FIX: Define buffer as a char array
    char buffer[256];
    size_t len = serializeJson(doc, buffer);

    ws.textAll(buffer, len);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, data, len);
        if (error) {
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.c_str());
            return;
        }

        if (doc.containsKey("state")) {
            config.state = doc["state"].as<bool>();
        }
        if (doc.containsKey("brightness")) {
            config.brightness = doc["brightness"].as<int>();
        }
        if (doc.containsKey("timerEnabled")) {
            config.timerEnabled = doc["timerEnabled"].as<bool>();
            setupTimers(); // Re-evaluate timers
        }
        // FIX: Correctly get the string value from the JSON doc before copying
        if (doc.containsKey("onTime")) {
            strlcpy(config.onTime, doc["onTime"], sizeof(config.onTime));
            setupTimers(); // Re-evaluate timers
        }
        if (doc.containsKey("offTime")) {
            strlcpy(config.offTime, doc["offTime"], sizeof(config.offTime));
            setupTimers(); // Re-evaluate timers
        }

        setLed(config.state, config.brightness);
        shouldSaveConfig = true; // Flag to save changes
        notifyClients();
    }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            notifyClients(); // Send current state to newly connected client
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void setLed(bool newState, int newBrightness) {
    config.state = newState;
    config.brightness = newBrightness;

    if (!config.state) { // If state is OFF
        analogWrite(PWM_OUTPUT_PIN, PWM_RANGE); // BJT ON -> MOSFET Gate LOW -> LED OFF
    } else {
        int pwmValue = PWM_RANGE - map(newBrightness, 0, 255, 0, PWM_RANGE); // Invert brightness for the driver
        analogWrite(PWM_OUTPUT_PIN, pwmValue); // BJT OFF -> MOSFET Gate HIGH -> LED ON (at brightness)
    }
}


// =================================================================
// Time and Timer (Scheduler) Functions
// =================================================================

void setupTime() {
    // As you're in Pune, India, the timezone is IST (UTC+5:30)
    // seconds from UTC = 5.5 * 3600 = 19800
    configTime(19800, 0, "pool.ntp.org", "time.nist.gov");
    Serial.print("Waiting for NTP time sync: ");
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    Serial.println("");
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    Serial.print("Current time: ");
    Serial.println(asctime(&timeinfo));
}

void digitalClockDisplay() {
  time_t now = time(nullptr);
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.printf("%02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

void triggerLedOn() {
    Serial.println("Timer: ON");
    if (config.timerEnabled) {
        setLed(true, config.brightness);
        notifyClients();
        shouldSaveConfig = true;
    }
}

void triggerLedOff() {
    Serial.println("Timer: OFF");
    if (config.timerEnabled) {
        setLed(false, config.brightness);
        notifyClients();
        shouldSaveConfig = true;
    }
}

void setupTimers() {
    // Free the old alarms before creating new ones.
    if (onTimerId != dtINVALID_ALARM_ID) {
        Alarm.free(onTimerId);
    }
    if (offTimerId != dtINVALID_ALARM_ID) {
        Alarm.free(offTimerId);
    }

    if (config.timerEnabled) {
        int onHour, onMin, offHour, offMin;
        sscanf(config.onTime, "%d:%d", &onHour, &onMin);
        sscanf(config.offTime, "%d:%d", &offHour, &offMin);

        // Store the new alarm IDs
        onTimerId = Alarm.alarmRepeat(onHour, onMin, 0, triggerLedOn);
        offTimerId = Alarm.alarmRepeat(offHour, offMin, 0, triggerLedOff);
        
        Serial.printf("Timers set. ON: %s (ID: %d), OFF: %s (ID: %d)\n", config.onTime, onTimerId, offTimerId);
    } else {
        Serial.println("Timers disabled.");
    }
}


// =================================================================
// Setup
// =================================================================

void setup() {
    Serial.begin(115200);
    Serial.println("\n\nBooting...");

    pinMode(PWM_OUTPUT_PIN, OUTPUT);
    pinMode(CONFIG_RESET_PIN, INPUT_PULLUP);
    analogWriteRange(PWM_RANGE);

    if (!LittleFS.begin()) {
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }
    
    loadConfiguration("/config.json", config);
    setLed(config.state, config.brightness);

    // WiFiManager Setup
    ESPAsync_WMParameter custom_device_name("deviceName", "Device Name", config.deviceName, 32);
    wifiManager.addParameter(&custom_device_name);
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.setConfigPortalTimeout(180); // 3 minutes

    if (digitalRead(CONFIG_RESET_PIN) == LOW) {
        Serial.println("Reset pin is active. Starting configuration portal.");
        if (!wifiManager.startConfigPortal("LED-Bar-Setup")) {
            Serial.println("Failed to connect and hit timeout");
            delay(3000);
            ESP.restart();
        }
    } else {
        wifiManager.setConnectTimeout(30);
        if (!wifiManager.autoConnect("LED-Bar-Setup")) {
            Serial.println("Failed to connect to saved WiFi. Restarting.");
            delay(3000);
            ESP.restart();
        }
    }

    if (shouldSaveConfig) {
        strlcpy(config.deviceName, custom_device_name.getValue(), sizeof(config.deviceName));
        saveConfiguration("/config.json", config);
    }
    
    WiFi.hostname(config.deviceName); 
    if (MDNS.begin(config.deviceName)) {
        Serial.printf("mDNS responder started. Access at http://%s.local\n", config.deviceName);
        MDNS.addService("http", "tcp", 80);
    } else {
        Serial.println("Error setting up MDNS responder!");
    }

    setupTime();
    setupTimers();
    ws.onEvent(onEvent);
    server.addHandler(&ws);

    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    
    // FIX: Corrected lambda syntax
    server.onNotFound([](AsyncWebServerRequest *request){
        request->send(404, "text/plain", "Not found");
    });

    server.begin();
    Serial.println("HTTP server started.");
}

// =================================================================
// Main Loop
// =================================================================
long lastSave = 0;
void loop() {
    ws.cleanupClients();
    MDNS.update();
    Alarm.delay(1000); // Process timers

    // Debounced save
    if (shouldSaveConfig && (millis() - lastSave > 5000)) {
        saveConfiguration("/config.json", config);
        shouldSaveConfig = false;
        lastSave = millis();
    }

    if (digitalRead(CONFIG_RESET_PIN) == LOW) {
        delay(100); // Debounce
        if (digitalRead(CONFIG_RESET_PIN) == LOW) {
            Serial.println("Reset pin activated. Restarting and entering config mode.");
            wifiManager.resetSettings();
            delay(1000);
            ESP.restart();
        }
    }
}