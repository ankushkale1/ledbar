#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h" // Required for advanced protocol and power control

// --- Configuration ---
// Note: LED_BUILTIN often maps to GPIO 2 on standard ESP32 boards,
// or GPIO 7 on some ESP32-C3 mini modules. Check your board documentation.
#define LED_PIN LED_BUILTIN

// #define ESP32_WIFI_TX_POWER WIFI_POWER_5dBm

// --- IMPORTANT: Update these credentials ---
const char *ssid = "JioFiber-4G";        // Ensure this is the correct, case-sensitive SSID
const char *password = "Ak@00789101112"; // Ensure this is the correct, case-sensitive Password

// --- Configuration Constants for Stability ---
// Reduces Max Transmission (Tx) Power to help mitigate Brownout errors on small boards.[3]
const int MAX_TX_POWER_DBM = WIFI_POWER_15dBm;

// --- Variables for Connection Monitoring and LED Blinking ---
bool led_on = false;
unsigned long previousBlinkMillis = 0;
const long blinkInterval = 100; // Blink rate in milliseconds while connecting

// --- Function to translate Wi-Fi Status Codes for clear debugging ---
// Uses the standard wl_status_t enumeration [5]
const char *wifiStatusToString(uint8_t status)
{
  switch (status)
  {
  case WL_IDLE_STATUS:
    return "0 - WL_IDLE_STATUS (Waiting)";
  case WL_NO_SSID_AVAIL:
    return "1 - WL_NO_SSID_AVAIL (SSID/2.4 GHz Issue)";
  case WL_CONNECTED:
    return "3 - WL_CONNECTED (Success)";
  case WL_CONNECT_FAILED:
    return "4 - WL_CONNECT_FAILED (Incorrect Password/Auth Fail)";
  case WL_CONNECTION_LOST:
    return "5 - WL_CONNECTION_LOST (Signal Drop)";
  case WL_DISCONNECTED:
    return "6 - WL_DISCONNECTED (Stalled or Firmware Error)";
  case WL_SCAN_COMPLETED:
    return "2 - WL_SCAN_COMPLETED";
  case WL_NO_SHIELD:
    return "255 - WL_NO_SHIELD";
  default:
    return "Unknown Status or ESP32-Specific Error";
  }
}

// --- Function to manage the LED blinking during connection attempts ---
void blinkLED()
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousBlinkMillis >= blinkInterval)
  {
    previousBlinkMillis = currentMillis;
    led_on = !led_on;
    digitalWrite(LED_PIN, led_on);
  }
}

// --- Robust Connection Function with Hardware & Software Mitigation ---
uint8_t initWiFi()
{
  Serial.println("Starting Robust Wi-Fi Connection Routine...");

  // 1. HARDWARE MITIGATION: Reduce Max Transmission (Tx) Power
  // esp_wifi_set_max_tx_power(WIFI_POWER_8_5dBm);
  // Serial.printf("Set Max TX Power to %d dBm.\n", MAX_TX_POWER_DBM);
  // 2. PROTOCOL HARDENING: Limit Wi-Fi Protocol to stabilize connection
  // Limits power demands and avoids issues with modern 802.11N/router steering.[4]
  // esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G);
  // Serial.println("Limited Wi-Fi Protocol to 802.11B/G.");

  // --- Standard Initialization ---
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true); // Clean disconnect for a fresh start [6]
  delay(100);

  Serial.print("Attempting connection to network: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  // WiFi.setTxPower(WIFI_POWER_8_5dBm);

  // 3. LED BLINKING while waiting for connection
  unsigned long connectionStartMillis = millis();

  // Wait for the internal Wi-Fi stack to stabilize before calling the definitive result function
  while (WiFi.status() == WL_IDLE_STATUS)
  {
    blinkLED();
    delay(50);
    // Add a sanity check break if we spend too long idling (e.g., 5 seconds)
    if (millis() - connectionStartMillis > 5000)
      break;
  }

  // Use the reliable waitForConnectResult() to resolve status [1, 2]
  uint8_t final_status = WiFi.waitForConnectResult();

  // Turn LED OFF after connection attempt, will be turned ON below if successful
  digitalWrite(LED_PIN, LOW);

  // --- Final Status Evaluation and Report ---
  if (final_status == WL_CONNECTED)
  {
    Serial.println("\n--- WiFi connected successfully ---");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    // LED ON solid for successful connection
    digitalWrite(LED_PIN, HIGH);
  }
  else
  {
    Serial.println("\n--- WiFi Connection FAILED ---");
    Serial.print("Final Reason: ");
    Serial.println(wifiStatusToString(final_status));
    Serial.println("Check network settings (2.4GHz) or power stability (Brownout).");
  }
  return final_status;
}

void msetup()
{
  Serial.begin(115200);
  delay(100);

  // Set up the LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Start with LED OFF

  Serial.println("System Initialized.");

  // Start connection attempt
  initWiFi();
}

void mloop()
{
  // --- Connection Monitoring and Reconnection Logic ---
  if (WiFi.status() != WL_CONNECTED)
  {
    static unsigned long previousReconnectMillis = 0;
    const long reconnectInterval = 10000; // Attempt reconnection every 10 seconds

    unsigned long currentMillis = millis();

    // 1. Maintain LED OFF while disconnected (or in error state)
    digitalWrite(LED_PIN, LOW);

    // 2. Check if it's time to attempt reconnection
    if (currentMillis - previousReconnectMillis >= reconnectInterval)
    {
      previousReconnectMillis = currentMillis;
      Serial.print("Connection lost (Status: ");
      Serial.print((int)WiFi.status());
      Serial.println("). Attempting controlled reconnection...");

      // Attempt reconnection - use the full initWiFi function to re-run all setup and checks
      initWiFi();
    }
  }
  else
  {
    // 3. Keep LED ON solid while connection is maintained [10]
    digitalWrite(LED_PIN, HIGH);
  }
}