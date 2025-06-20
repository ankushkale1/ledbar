// =================================================================
//                 LIBRARY INCLUSIONS
// =================================================================
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WiFiManager.h>

// =================================================================
//                 PIN & CONSTANT DEFINITIONS
// =================================================================
const int PWM_OUTPUT_PIN = D1;      // GPIO5 on NodeMCU
const int CONFIG_TRIGGER_PIN = D2;  // GPIO4, a safe pin for boot checking
const int PWM_RANGE = 255;          // Using 8-bit PWM

// =================================================================
//                 GLOBAL VARIABLES & OBJECTS
// =================================================================

// --- Configuration Struct ---
struct Config {
  char deviceName = "ledbar";
  bool ledState = false;
  int brightness = 128;
  bool timerEnabled = false;
  int onHour = 8;
  int onMinute = 0;
  int offHour = 22;
  int offMinute = 0;
};
Config config;
bool shouldSaveConfig = false;

// --- Web Server & WebSockets ---
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// --- NTP Time Client ---
const long UTC_OFFSET_IN_SECONDS = 0; // TODO: Set your timezone offset
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", UTC_OFFSET_IN_SECONDS);

// --- Scheduler State ---
int lastCheckedMinute = -1;
bool onActionTriggered = false;
bool offActionTriggered = false;

// =================================================================
//                 FORWARD DECLARATIONS
// =================================================================
void setLedOutput(bool state, int brightness);
void notifyClients();
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void loadConfiguration(const char *filename, Config &config);
void saveConfiguration(const char *filename, const Config &config);
void saveConfigCallback();

// =================================================================
//                 LED CONTROL LOGIC
// =================================================================
// Sets the LED state and brightness, handling the inverting driver logic
void setLedOutput(bool state, int brightness) {
  if (state) {
    // Inverting driver: ESP PWM 0 = MAX Brightness, 255 = OFF
    // To get target brightness B, we need ESP PWM = 255 - B
    int pwmValue = PWM_RANGE - constrain(brightness, 0, PWM_RANGE);
    analogWrite(PWM_OUTPUT_PIN, pwmValue);
  } else {
    // LED OFF
    analogWrite(PWM_OUTPUT_PIN, PWM_RANGE);
  }
}

// =================================================================
//                 PERSISTENCE (LittleFS & JSON)
// =================================================================
// Loads configuration from a JSON file on LittleFS
void loadConfiguration(const char *filename, Config &config) {
  if (LittleFS.begin()) {
    File configFile = LittleFS.open(filename, "r");
    if (!configFile) {
      Serial.println("Failed to open config file for reading, using defaults.");
      saveConfiguration(filename, config); // Create file with defaults
      return;
    }

    StaticJsonDocument doc;
    DeserializationError error = deserializeJson(doc, configFile);
    if (error) {
      Serial.println("Failed to parse config file, using defaults.");
      return;
    }

    strlcpy(config.deviceName, doc["deviceName"] | "ledbar", sizeof(config.deviceName));
    config.ledState = doc | false;
    config.brightness = doc["brightness"] | 128;
    config.timerEnabled = doc["timerEnabled"] | false;
    config.onHour = doc["onHour"] | 8;
    config.onMinute = doc["onMinute"] | 0;
    config.offHour = doc["offHour"] | 22;
    config.offMinute = doc["offMinute"] | 0;

    configFile.close();
    Serial.println("Configuration loaded.");
  } else {
    Serial.println("Failed to mount LittleFS.");
  }
}

// Saves configuration to a JSON file on LittleFS
void saveConfiguration(const char *filename, const Config &config) {
  File configFile = LittleFS.open(filename, "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing.");
    return;
  }

  StaticJsonDocument doc;
  doc["deviceName"] = config.deviceName;
  doc = config.ledState;
  doc["brightness"] = config.brightness;
  doc["timerEnabled"] = config.timerEnabled;
  doc["onHour"] = config.onHour;
  doc["onMinute"] = config.onMinute;
  doc["offHour"] = config.offHour;
  doc["offMinute"] = config.offMinute;

  if (serializeJson(doc, configFile) == 0) {
    Serial.println("Failed to write to config file.");
  } else {
    Serial.println("Configuration saved.");
  }
  configFile.close();
}

// =================================================================
//                 WEBSOCKETS LOGIC
// =================================================================
// Sends current device state to all connected WebSocket clients
void notifyClients() {
  StaticJsonDocument doc;
  doc["action"] = "update";
  doc = config.ledState;
  doc["brightness"] = config.brightness;
  doc["timerEnabled"] = config.timerEnabled;
  doc["onHour"] = config.onHour;
  doc["onMinute"] = config.onMinute;
  doc["offHour"] = config.offHour;
  doc["offMinute"] = config.offMinute;
  
  char buffer;
  size_t len = serializeJson(doc, buffer);
  ws.textAll(buffer, len);
}

// Handles incoming WebSocket messages
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    StaticJsonDocument doc;
    DeserializationError error = deserializeJson(doc, (char*)data);
    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }

    const char* action = doc["action"];

    if (strcmp(action, "toggle") == 0) {
      config.ledState = doc["state"];
      setLedOutput(config.ledState, config.brightness);
      saveConfiguration("/config.json", config);
    } else if (strcmp(action, "brightness") == 0) {
      config.brightness = doc["value"];
      if (config.ledState) {
        setLedOutput(true, config.brightness);
      }
      saveConfiguration("/config.json", config);
    } else if (strcmp(action, "updateTimer") == 0) {
      config.timerEnabled = doc["enabled"];
      config.onHour = doc["onHour"];
      config.onMinute = doc["onMinute"];
      config.offHour = doc["offHour"];
      config.offMinute = doc["offMinute"];
      saveConfiguration("/config.json", config);
    }
    
    notifyClients(); // Notify all clients of the change
  }
}

// WebSocket event handler
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      // Send current state to the newly connected client
      notifyClients();
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

// =================================================================
//                 WiFiManager CALLBACK
// =================================================================
void saveConfigCallback() {
  Serial.println("Entered save config callback");
  shouldSaveConfig = true;
}

// =================================================================
//                 SETUP FUNCTION
// =================================================================
void setup() {
  Serial.begin(115200);
  pinMode(PWM_OUTPUT_PIN, OUTPUT);
  pinMode(CONFIG_TRIGGER_PIN, INPUT_PULLUP);

  // --- Filesystem & Config Loading ---
  loadConfiguration("/config.json", config);

  // --- WiFiManager Setup ---
  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  
  // Add custom parameter for device name
  WiFiManagerParameter custom_device_name("deviceName", "Device Name", config.deviceName, 32);
  wifiManager.addParameter(&custom_device_name);

  // Set a timeout for the portal
  wifiManager.setConfigPortalTimeout(180);

  // Check if config portal is requested
  if (digitalRead(CONFIG_TRIGGER_PIN) == LOW) {
    Serial.println("Config button pressed. Starting portal.");
    if (!wifiManager.startConfigPortal("LEDBar-Setup")) {
      Serial.println("Failed to connect and hit timeout");
      ESP.restart();
    }
  } else {
    // Set hostname before connecting
    WiFi.hostname(config.deviceName);
    wifiManager.autoConnect("LEDBar-Setup");
  }

  // --- Save config if changed via portal ---
  if (shouldSaveConfig) {
    strcpy(config.deviceName, custom_device_name.getValue());
    saveConfiguration("/config.json", config);
  }

  // --- Post-Connection Setup ---
  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Hostname: ");
  Serial.println(WiFi.hostname());

  // --- mDNS Setup ---
  if (MDNS.begin(config.deviceName)) {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("mDNS responder started. Access at http://%s.local\n", config.deviceName);
  }

  // --- NTP Client Setup ---
  timeClient.begin();

  // --- Web Server & WebSocket Setup ---
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET,(AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.serveStatic("/", LittleFS, "/");

  server.begin();
  Serial.println("HTTP server started.");

  // --- Set initial LED state ---
  setLedOutput(config.ledState, config.brightness);
}

// =================================================================
//                 MAIN LOOP
// =================================================================
void loop() {
  ws.cleanupClients();
  MDNS.update();

  // --- Scheduler Logic (runs roughly every second) ---
  static unsigned long lastTimeCheck = 0;
  if (millis() - lastTimeCheck > 1000) {
    lastTimeCheck = millis();
    
    if (WiFi.status() == WL_CONNECTED) {
      timeClient.update();
      
      int currentHour = timeClient.getHours();
      int currentMinute = timeClient.getMinutes();

      // Reset daily triggers at midnight
      if (currentHour == 0 && currentMinute == 0) {
        onActionTriggered = false;
        offActionTriggered = false;
      }

      if (config.timerEnabled) {
        // Check for ON time
        if (currentHour == config.onHour && currentMinute == config.onMinute &&!onActionTriggered) {
          if (!config.ledState) { // Only act if it's currently off
            Serial.println("Timer: Turning LED ON");
            config.ledState = true;
            setLedOutput(true, config.brightness);
            saveConfiguration("/config.json", config);
            notifyClients();
          }
          onActionTriggered = true;
        }

        // Check for OFF time
        if (currentHour == config.offHour && currentMinute == config.offMinute &&!offActionTriggered) {
          if (config.ledState) { // Only act if it's currently on
            Serial.println("Timer: Turning LED OFF");
            config.ledState = false;
            setLedOutput(false, 0);
            saveConfiguration("/config.json", config);
            notifyClients();
          }
          offActionTriggered = true;
        }
      }
    }
  }
}