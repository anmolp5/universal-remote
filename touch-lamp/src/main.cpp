#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include "secrets.h"
#include "hardware_config.h"
#include "web_ui.h"

// =============================================================================
// HTTP Web Server & Cloud MQTT Client
// =============================================================================
ESP8266WebServer server(HTTP_PORT);
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

// Timing trackers
static unsigned long lastWifiCheck = 0;
const unsigned long WIFI_CHECK_INTERVAL = 10000;
static unsigned long lastMqttReconnect = 0;
const unsigned long MQTT_RECONNECT_INTERVAL = 5000;
static bool servicesStarted = false;

// =============================================================================
// Pulse Queue & Non-Blocking State Machine
// =============================================================================
enum PulseState {
  PULSE_IDLE,
  PULSE_ACTIVE,
  PULSE_GAP,
  PULSE_VERIFY_OFF
};

struct PulseQueueManager {
  PulseState state = PULSE_IDLE;
  int remainingTaps = 0;
  bool verifyOffWhenDone = false;
  int verifyRetries = 0;
  unsigned long stateStartTime = 0;
} pQueue;

// =============================================================================
// Closed-Loop State Sensing Variables
// =============================================================================
// Standby LED lit (HIGH / 2.6V) -> Lamp is OFF
// Standby LED dark (LOW / 0V)   -> Lamp is ON
static bool lastRawSenseReading = HIGH;
static bool currentLampState = false; // true = ON, false = OFF
static unsigned long lastDebounceTime = 0;

// Forward declarations
bool isLampOn();
void publishStatus();
bool turnOn();
bool turnOff();
bool singleTap();
void updatePulseQueue();
void updateSensor();
void setupWiFi();
void checkWiFiConnection();
void setupMqtt();
void checkMqttConnection();
void startNetworkServices();

// =============================================================================
// Lamp State Helper
// =============================================================================
bool isLampOn() {
  // Standby LED anode is LOW when lamp is ON
  return (digitalRead(PIN_STATE_SENSE) == LOW);
}

void publishStatus() {
  if (mqttClient.connected()) {
    String payload = currentLampState ? "{\"state\":\"ON\"}" : "{\"state\":\"OFF\"}";
    mqttClient.publish(MQTT_TOPIC_STATUS, payload.c_str(), true);
    Serial.printf("[MQTT TX] Status -> %s\n", payload.c_str());
  }
}

// =============================================================================
// Actuation Functions
// =============================================================================
bool turnOn() {
  if (isLampOn()) {
    Serial.println(F("[CTRL] Turn ON requested, but lamp is already ON."));
    publishStatus();
    return true;
  }
  Serial.println(F("[CTRL] Turning ON (2 taps to Warm White)..."));
  pQueue.remainingTaps = 2;
  pQueue.verifyOffWhenDone = false;
  pQueue.verifyRetries = 0;
  pQueue.state = PULSE_ACTIVE;
  pQueue.stateStartTime = millis();
  digitalWrite(PIN_TOUCH_CTRL, HIGH);
  return true;
}

bool turnOff() {
  if (!isLampOn()) {
    Serial.println(F("[CTRL] Turn OFF requested, but lamp is already OFF."));
    publishStatus();
    return true;
  }
  Serial.println(F("[CTRL] Turning OFF (2 taps + closed-loop verify)..."));
  pQueue.remainingTaps = 2;
  pQueue.verifyOffWhenDone = true;
  pQueue.verifyRetries = 0;
  pQueue.state = PULSE_ACTIVE;
  pQueue.stateStartTime = millis();
  digitalWrite(PIN_TOUCH_CTRL, HIGH);
  return true;
}

bool singleTap() {
  Serial.println(F("[CTRL] Single Tap (1x) pulse..."));
  pQueue.remainingTaps = 1;
  pQueue.verifyOffWhenDone = false;
  pQueue.verifyRetries = 0;
  pQueue.state = PULSE_ACTIVE;
  pQueue.stateStartTime = millis();
  digitalWrite(PIN_TOUCH_CTRL, HIGH);
  return true;
}

// =============================================================================
// Non-Blocking Pulse Queue Engine
// =============================================================================
void updatePulseQueue() {
  unsigned long now = millis();

  switch (pQueue.state) {
    case PULSE_IDLE:
      break;

    case PULSE_ACTIVE:
      if (now - pQueue.stateStartTime >= PULSE_TAP_MS) {
        digitalWrite(PIN_TOUCH_CTRL, LOW);
        pQueue.remainingTaps--;
        pQueue.stateStartTime = now;

        if (pQueue.remainingTaps > 0) {
          pQueue.state = PULSE_GAP;
        } else {
          if (pQueue.verifyOffWhenDone) {
            pQueue.state = PULSE_VERIFY_OFF;
          } else {
            pQueue.state = PULSE_IDLE;
          }
        }
      }
      break;

    case PULSE_GAP:
      if (now - pQueue.stateStartTime >= INTER_TAP_GAP_MS) {
        digitalWrite(PIN_TOUCH_CTRL, HIGH);
        pQueue.state = PULSE_ACTIVE;
        pQueue.stateStartTime = now;
      }
      break;

    case PULSE_VERIFY_OFF:
      // Allow 180ms settling time for lamp driver & LED
      if (now - pQueue.stateStartTime >= 180) {
        if (!isLampOn()) {
          Serial.println(F("[OFF] Closed-loop verified: Standby LED lit, Lamp is OFF."));
          pQueue.state = PULSE_IDLE;
          pQueue.verifyOffWhenDone = false;
        } else {
          if (pQueue.verifyRetries < 4) {
            pQueue.verifyRetries++;
            Serial.printf("[OFF] Lamp still ON! Corrective tap %d/4...\n", pQueue.verifyRetries);
            pQueue.remainingTaps = 1;
            pQueue.state = PULSE_ACTIVE;
            pQueue.stateStartTime = now;
            digitalWrite(PIN_TOUCH_CTRL, HIGH);
          } else {
            Serial.println(F("[OFF] Max corrective retries reached."));
            pQueue.state = PULSE_IDLE;
            pQueue.verifyOffWhenDone = false;
          }
        }
      }
      break;
  }
}

// =============================================================================
// Debounced Sensor Monitoring (Physical Tap Sync)
// =============================================================================
void updateSensor() {
  bool rawReading = digitalRead(PIN_STATE_SENSE);

  if (rawReading != lastRawSenseReading) {
    lastDebounceTime = millis();
    lastRawSenseReading = rawReading;
  }

  if ((millis() - lastDebounceTime) > SENSE_DEBOUNCE_MS) {
    bool detectedState = (rawReading == LOW); // LOW = dark = ON
    if (detectedState != currentLampState) {
      currentLampState = detectedState;
      Serial.printf("[SENSE] State changed: %s (Standby LED: %s)\n",
                    currentLampState ? "ON" : "OFF",
                    rawReading == HIGH ? "LIT" : "DARK");
      publishStatus();
    }
  }
}

// =============================================================================
// Web API Handlers
// =============================================================================
void sendCORSHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void handleRoot() {
  sendCORSHeaders();
  server.send(200, "text/html", INDEX_HTML);
}

void handleApiStatus() {
  sendCORSHeaders();
  String json = "{";
  json += "\"status\":\"ok\",";
  json += "\"state\":\"" + String(currentLampState ? "ON" : "OFF") + "\",";
  json += "\"sensor_raw\":" + String(digitalRead(PIN_STATE_SENSE)) + ",";
  json += "\"uptime_s\":" + String(millis() / 1000);
  json += "}";
  server.send(200, "application/json", json);
}

void handleApiOn() {
  sendCORSHeaders();
  turnOn();
  server.send(200, "application/json", "{\"status\":\"ok\",\"action\":\"on\"}");
}

void handleApiOff() {
  sendCORSHeaders();
  turnOff();
  server.send(200, "application/json", "{\"status\":\"ok\",\"action\":\"off\"}");
}

void handleApiTap() {
  sendCORSHeaders();
  singleTap();
  server.send(200, "application/json", "{\"status\":\"ok\",\"action\":\"tap\"}");
}

void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/lamp/status", HTTP_GET, handleApiStatus);
  server.on("/api/lamp/on", HTTP_ANY, handleApiOn);
  server.on("/api/lamp/off", HTTP_ANY, handleApiOff);
  server.on("/api/lamp/tap", HTTP_ANY, handleApiTap);

  server.onNotFound([]() {
    sendCORSHeaders();
    if (server.method() == HTTP_OPTIONS) {
      server.send(204);
      return;
    }
    server.send(404, "application/json", "{\"error\":\"Not found\"}");
  });

  server.begin();
  Serial.println(F("[HTTP] Web server listening on port 80"));
}

// =============================================================================
// ArduinoOTA Setup
// =============================================================================
void setupOTA() {
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname(DEVICE_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onStart([]() {
    Serial.println(F("[OTA] Update Starting..."));
  });
  ArduinoOTA.onEnd([]() {
    Serial.println(F("\n[OTA] Update Complete!"));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println(F("Auth Failed"));
    else if (error == OTA_BEGIN_ERROR) Serial.println(F("Begin Failed"));
    else if (error == OTA_CONNECT_ERROR) Serial.println(F("Connect Failed"));
    else if (error == OTA_RECEIVE_ERROR) Serial.println(F("Receive Failed"));
    else if (error == OTA_END_ERROR) Serial.println(F("End Failed"));
  });

  ArduinoOTA.begin();
  Serial.println(F("[OTA] ArduinoOTA service initialized (port 8266)"));
}

// =============================================================================
// HiveMQ Cloud MQTT Implementation
// =============================================================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();
  Serial.printf("[MQTT RX] %s -> %s\n", topic, message.c_str());

  String t = String(topic);
  if (t == MQTT_TOPIC_COMMAND) {
    if (message.equalsIgnoreCase("ON") || message == "1") {
      turnOn();
    } else if (message.equalsIgnoreCase("OFF") || message == "0") {
      turnOff();
    } else if (message.equalsIgnoreCase("TAP") || message.equalsIgnoreCase("CYCLE") || message.equalsIgnoreCase("TOGGLE")) {
      singleTap();
    }
  } else if (t == MQTT_TOPIC_MASTER_CMD) {
    if (message == "all_on") {
      turnOn();
    } else if (message == "all_off") {
      turnOff();
    }
  }
}

void setupMqtt() {
  espClient.setInsecure();
  espClient.setBufferSizes(512, 512);

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(512);

  Serial.printf("[MQTT] Configured for HiveMQ Cloud (%s:%d)\n", MQTT_HOST, MQTT_PORT);
}

void checkMqttConnection() {
  if (WiFi.status() != WL_CONNECTED) return;

  if (mqttClient.connected()) {
    mqttClient.loop();
    return;
  }

  unsigned long now = millis();
  if (now - lastMqttReconnect >= MQTT_RECONNECT_INTERVAL) {
    lastMqttReconnect = now;
    Serial.print(F("[MQTT] Connecting to HiveMQ Cloud... "));

    String clientId = String(DEVICE_HOSTNAME) + "-" + String(ESP.getChipId(), HEX);

    bool connected = mqttClient.connect(
      clientId.c_str(),
      MQTT_USER,
      MQTT_PASS,
      MQTT_TOPIC_AVAILABILITY,
      0,
      true,
      "offline"
    );

    if (connected) {
      Serial.println(F("Connected!"));
      mqttClient.publish(MQTT_TOPIC_AVAILABILITY, "online", true);
      publishStatus();
      mqttClient.subscribe(MQTT_TOPIC_COMMAND);
      mqttClient.subscribe(MQTT_TOPIC_MASTER_CMD);
      Serial.printf("[MQTT] Subscribed to %s and %s\n", MQTT_TOPIC_COMMAND, MQTT_TOPIC_MASTER_CMD);
    } else {
      Serial.printf("Failed (rc=%d). Retrying in %lu s.\n", mqttClient.state(), MQTT_RECONNECT_INTERVAL / 1000);
    }
  }
}

// =============================================================================
// Network Services Initialization
// =============================================================================
void startNetworkServices() {
  if (servicesStarted) return;
  servicesStarted = true;

  if (MDNS.begin(DEVICE_HOSTNAME)) {
    MDNS.addService("http", "tcp", HTTP_PORT);
    Serial.printf("[mDNS] Active at http://%s.local\n", DEVICE_HOSTNAME);
  }

  setupOTA();
  setupWebServer();
  setupMqtt();
}

void setupWiFi() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(DEVICE_HOSTNAME);
  WiFi.setAutoReconnect(true);

  Serial.printf("[WiFi] Connecting to SSID: \"%s\"...\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 20000) {
    delay(400);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F("[WiFi] Connected! IP Address: "));
    Serial.println(WiFi.localIP());
    startNetworkServices();
  } else {
    Serial.printf("[WiFi] Still connecting in background (status=%d)...\n", WiFi.status());
  }
}

void checkWiFiConnection() {
  unsigned long now = millis();
  if (now - lastWifiCheck >= WIFI_CHECK_INTERVAL) {
    lastWifiCheck = now;
    if (WiFi.status() == WL_CONNECTED && !servicesStarted) {
      Serial.print(F("[WiFi] Reconnected! IP: "));
      Serial.println(WiFi.localIP());
      startNetworkServices();
    }
  }
}

// =============================================================================
// Arduino Setup & Loop
// =============================================================================
void setup() {
  // CRITICAL: Ensure touch control pin starts strictly LOW before anything else
  pinMode(PIN_TOUCH_CTRL, OUTPUT);
  digitalWrite(PIN_TOUCH_CTRL, LOW);

  // Closed-loop standby LED sensor pin
  pinMode(PIN_STATE_SENSE, INPUT);

  // Read initial lamp state
  currentLampState = isLampOn();
  lastRawSenseReading = digitalRead(PIN_STATE_SENSE);

  Serial.begin(115200);
  delay(200);

  Serial.println(F("\n====================================================="));
  Serial.println(F("   Desk Lamp Smart Controller (ESP8266) Ready        "));
  Serial.println(F("====================================================="));
  Serial.printf("Touch Pin (D2): GPIO %d | Sense Pin (D5): GPIO %d\n", PIN_TOUCH_CTRL, PIN_STATE_SENSE);
  Serial.printf("Initial Lamp State: %s\n", currentLampState ? "ON" : "OFF");
  Serial.println(F("====================================================="));

  setupWiFi();
}

void loop() {
  // Non-blocking pulse queue state machine
  updatePulseQueue();

  // Debounced closed-loop sensor monitoring
  updateSensor();

  // Wi-Fi & HiveMQ Cloud MQTT connection manager
  checkWiFiConnection();
  checkMqttConnection();

  // Handle incoming HTTP requests
  server.handleClient();

  // mDNS & OTA handlers
  MDNS.update();
  ArduinoOTA.handle();
}
