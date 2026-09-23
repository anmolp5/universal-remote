#include <Arduino.h>

#if defined(ESP32)
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "ble_manager.h"
#define WebServerType WebServer
#else
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#define WebServerType ESP8266WebServer
#endif

#include "secrets.h"
#include "hardware_config.h"
#include "command_queue.h"
#include "web_ui.h"

// HTTP Web Server on port 80
WebServerType server(HTTP_PORT);

// Cloud MQTT Client over TLS (HiveMQ Cloud)
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

// Timing tracker for non-blocking Wi-Fi status check
static unsigned long lastWifiCheck = 0;
const unsigned long WIFI_CHECK_INTERVAL = 10000;

// Timing tracker for non-blocking MQTT reconnect
static unsigned long lastMqttReconnect = 0;
const unsigned long MQTT_RECONNECT_INTERVAL = 5000;

// Forward declarations
bool isMqttConfigured();
void setupMqtt();
void checkMqttConnection();

void triggerDeskLamp(const char* cmd) {
  Serial.printf("[Hub -> Desk] Forwarding command: %s\n", cmd);

  // 1. MQTT Cloud Publish
  if (mqttClient.connected()) {
    mqttClient.publish("home/desk_lamp/command", cmd);
  }

  // 2. Direct LAN HTTP call
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    client.setTimeout(200);
    if (client.connect("192.168.10.19", 80)) {
      String cmdLower = String(cmd);
      cmdLower.toLowerCase();
      client.print(String("POST /api/lamp/") + cmdLower + " HTTP/1.1\r\n" +
                   "Host: 192.168.10.19\r\n" +
                   "Connection: close\r\n\r\n");
      client.stop();
    }
  }
}

void printMenu() {
  Serial.println(F("\n====================================================="));
#if defined(ESP32)
  Serial.println(F("   Universal IR, RF & BLE Lighting Hub (ESP32) Ready "));
#else
  Serial.println(F("   Universal IR & RF Lighting Hub (ESP8266) Ready    "));
#endif
  Serial.println(F("====================================================="));
  Serial.println(F("Poster Light (IR NEC):"));
  Serial.println(F("  1 : ON | 2 : OFF | 3 : Color"));
  Serial.println(F("Donut Lamp (BLE ELK-BLEDOM07 / IR):"));
  Serial.println(F("  4 : ON | 5 : OFF"));
  Serial.println(F("Bedside Lamp (IR Pulse Distance):"));
  Serial.println(F("  6 : Power | 7 : Brighter | 8 : Dimmer | 9 : Warmer | 0 : Cooler"));
  Serial.println(F("Floor Lamp (Lotus Lamp BLE / RF):"));
  Serial.println(F("  q : Power | w : Warmer | e : Dimmer | r : Cooler | t : Brighter"));
  Serial.println(F("Desk Lamp (Smart Touch via ESP8266):"));
  Serial.println(F("  d : ON (Warm White) | x : OFF | c : Cycle / Tap"));
  Serial.println(F("Master Macros:"));
  Serial.println(F("  a : ALL ON | o : ALL OFF"));
  Serial.println(F("====================================================="));
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F("Web Interface: http://"));
    Serial.print(WiFi.localIP());
    Serial.println(F("/"));
    Serial.print(F("mDNS Domain:   http://"));
    Serial.print(F(DEVICE_HOSTNAME));
    Serial.println(F(".local/"));
    if (isMqttConfigured()) {
      Serial.print(F("Cloud MQTT:    "));
      Serial.println(mqttClient.connected() ? F("Connected (HiveMQ Cloud)") : F("Standby / Connecting"));
      Serial.print(F("MQTT Command:  "));
      Serial.println(F(MQTT_TOPIC_COMMAND));
    } else {
      Serial.println(F("Cloud MQTT:    Not configured (set credentials in include/secrets.h)"));
    }
  }
}

void setupWebServer() {
  // Serve the embedded responsive dark glassmorphism dashboard
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", INDEX_HTML);
  });

  // REST API: Trigger any single command queue action
  server.on("/api/action", HTTP_ANY, []() {
    if (!server.hasArg("cmd")) {
      server.send(400, "application/json", F("{\"error\":\"Missing cmd parameter\"}"));
      return;
    }

    String action = server.arg("cmd");
    bool dispatched = cmdQueue.dispatchAction(action);

    if (dispatched) {
      String res = String("{\"success\":true,\"action\":\"") + action + "\",\"states\":" + cmdQueue.getStatesJson() + "}";
      server.send(200, "application/json", res);
    } else {
      server.send(404, "application/json", "{\"error\":\"Unknown action\"}");
    }
  });

  // REST API: Query current assumed device states
  server.on("/api/status", HTTP_GET, []() {
    server.send(200, "application/json", cmdQueue.getStatesJson());
  });

  // REST API: Manual state resynchronization
  server.on("/api/resync", HTTP_POST, []() {
    if (!server.hasArg("device") || !server.hasArg("state")) {
      server.send(400, "application/json", "{\"error\":\"Missing device or state parameter\"}");
      return;
    }

    String device = server.arg("device");
    bool state = (server.arg("state").toInt() == 1);
    cmdQueue.setDeviceState(device, state);

    String res = String("{\"success\":true,\"states\":") + cmdQueue.getStatesJson() + "}";
    server.send(200, "application/json", res);
  });

  // REST API: Desk Lamp Controls
  server.on("/api/desk_on", HTTP_ANY, []() {
    triggerDeskLamp("ON");
    cmdQueue.setDeviceState("desk", true);
    server.send(200, "application/json", "{\"success\":true,\"action\":\"desk_on\"}");
  });

  server.on("/api/desk_off", HTTP_ANY, []() {
    triggerDeskLamp("OFF");
    cmdQueue.setDeviceState("desk", false);
    server.send(200, "application/json", "{\"success\":true,\"action\":\"desk_off\"}");
  });

  server.on("/api/desk_tap", HTTP_ANY, []() {
    triggerDeskLamp("TAP");
    server.send(200, "application/json", "{\"success\":true,\"action\":\"desk_tap\"}");
  });

#if defined(ESP32)
  // Floor Lamp BLE Controls
  server.on("/api/floor/color", HTTP_POST, []() {
    uint8_t r = server.hasArg("r") ? server.arg("r").toInt() : 255;
    uint8_t g = server.hasArg("g") ? server.arg("g").toInt() : 255;
    uint8_t b = server.hasArg("b") ? server.arg("b").toInt() : 255;
    bleManager.setFloorColor(r, g, b);
    server.send(200, "application/json", "{\"success\":true}");
  });

  server.on("/api/floor/brightness", HTTP_POST, []() {
    uint8_t val = server.hasArg("value") ? server.arg("value").toInt() : 100;
    bleManager.setFloorBrightness(val);
    server.send(200, "application/json", "{\"success\":true}");
  });

  server.on("/api/floor/cct", HTTP_POST, []() {
    uint8_t warm = server.hasArg("warm") ? server.arg("warm").toInt() : 50;
    uint8_t cool = server.hasArg("cool") ? server.arg("cool").toInt() : 50;
    bleManager.setFloorCCT(warm, cool);
    server.send(200, "application/json", "{\"success\":true}");
  });

  // Donut Lamp BLE Controls
  server.on("/api/donut/color", HTTP_POST, []() {
    uint8_t r = server.hasArg("r") ? server.arg("r").toInt() : 255;
    uint8_t g = server.hasArg("g") ? server.arg("g").toInt() : 255;
    uint8_t b = server.hasArg("b") ? server.arg("b").toInt() : 255;
    bleManager.setDonutColor(r, g, b);
    server.send(200, "application/json", "{\"success\":true}");
  });

  server.on("/api/donut/brightness", HTTP_POST, []() {
    uint8_t val = server.hasArg("value") ? server.arg("value").toInt() : 100;
    bleManager.setDonutBrightness(val);
    server.send(200, "application/json", "{\"success\":true}");
  });

  // Hybrid BLE Management API
  server.on("/api/ble/release", HTTP_POST, []() {
    bleManager.releaseBle();
    server.send(200, "application/json", "{\"success\":true,\"released\":true}");
  });

  server.on("/api/ble/reconnect", HTTP_POST, []() {
    bleManager.reconnectBle();
    server.send(200, "application/json", "{\"success\":true,\"released\":false}");
  });

  server.on("/api/ble/status", HTTP_GET, []() {
    String res = "{\"floorConnected\":" + String(bleManager.isFloorConnected() ? "true" : "false") +
                 ",\"donutConnected\":" + String(bleManager.isDonutConnected() ? "true" : "false") +
                 ",\"released\":" + String(bleManager.isReleased() ? "true" : "false") + "}";
    server.send(200, "application/json", res);
  });
#endif

  // REST API: Directly pulse any individual GPIO pin for hardware diagnosis
  server.on("/api/pulse_pin", HTTP_ANY, []() {
    if (!server.hasArg("pin")) {
      server.send(400, "application/json", F("{\"error\":\"Missing pin parameter\"}"));
      return;
    }
    uint8_t pin = server.arg("pin").toInt();
    cmdQueue.enqueueRFPulse(pin, "Direct GPIO Pulse");
    Serial.printf("[Diagnostic] Pulsing GPIO %d directly\n", pin);
    String res = String("{\"success\":true,\"pulsed_pin\":") + String(pin) + "}";
    server.send(200, "application/json", res);
  });

  server.onNotFound([]() {
    server.send(404, "text/plain", F("404: Not Found"));
  });

  server.begin();
  Serial.println(F("[HTTP] Web server started on port 80"));
}

void setupOTA() {
  ArduinoOTA.setHostname(DEVICE_HOSTNAME);
#ifdef OTA_PASSWORD
  ArduinoOTA.setPassword(OTA_PASSWORD);
#endif

  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.println("[OTA] Update starting (" + type + ")...");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println(F("\n[OTA] Update completed! Rebooting..."));
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA] Progress: %u%%\r", (progress / (total / 100)));
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
  Serial.println(F("[OTA] ArduinoOTA service initialized."));
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';

  String action = String(message);
  action.trim();

  Serial.printf("[MQTT] Inbound on [%s]: %s\n", topic, action.c_str());

  if (String(topic) == "home/desk_lamp/status") {
    bool isOn = (action.indexOf("\"ON\"") >= 0 || action.equalsIgnoreCase("ON"));
    cmdQueue.setDeviceState("desk", isOn);
    return;
  }

#if defined(ESP32)
  if (action.startsWith("FLOOR_COLOR:") || action.startsWith("floor_color:")) {
    String hex = action.substring(12);
    hex.replace("#", "");
    long num = strtol(hex.c_str(), NULL, 16);
    bleManager.setFloorColor((num >> 16) & 0xFF, (num >> 8) & 0xFF, num & 0xFF);
    return;
  }
  if (action.startsWith("FLOOR_CCT:") || action.startsWith("floor_cct:")) {
    String payload = action.substring(10);
    int commaIdx = payload.indexOf(',');
    if (commaIdx > 0) {
      uint8_t warm = payload.substring(0, commaIdx).toInt();
      uint8_t cool = payload.substring(commaIdx + 1).toInt();
      bleManager.setFloorCCT(warm, cool);
    }
    return;
  }
  if (action.startsWith("DONUT_COLOR:") || action.startsWith("donut_color:")) {
    String hex = action.substring(12);
    hex.replace("#", "");
    long num = strtol(hex.c_str(), NULL, 16);
    bleManager.setDonutColor((num >> 16) & 0xFF, (num >> 8) & 0xFF, num & 0xFF);
    return;
  }
  if (action.startsWith("FLOOR_BRIGHTNESS:") || action.startsWith("floor_brightness:")) {
    uint8_t val = action.substring(17).toInt();
    bleManager.setFloorBrightness(val);
    return;
  }
  if (action.startsWith("DONUT_BRIGHTNESS:") || action.startsWith("donut_brightness:")) {
    uint8_t val = action.substring(17).toInt();
    bleManager.setDonutBrightness(val);
    return;
  }
  if (action.equalsIgnoreCase("BLE_RELEASE")) {
    bleManager.releaseBle();
    return;
  }
  if (action.equalsIgnoreCase("BLE_RECONNECT")) {
    bleManager.reconnectBle();
    return;
  }
#endif

  if (String(topic) == MQTT_TOPIC_COMMAND) {
    bool ok = cmdQueue.dispatchAction(action);
    if (!ok) {
      Serial.printf("[MQTT] Unknown command payload: %s\n", action.c_str());
    }
  }
}

bool isMqttConfigured() {
  return (strlen(MQTT_HOST) > 0 &&
          strcmp(MQTT_HOST, "YOUR_CLUSTER_ID.s1.eu.hivemq.cloud") != 0 &&
          strcmp(MQTT_USER, "YOUR_MQTT_USERNAME") != 0);
}

void setupMqtt() {
  if (!isMqttConfigured()) {
    Serial.println(F("[MQTT] Not configured. Set credentials in include/secrets.h to enable."));
    return;
  }

  // Low-RAM TLS without heavy x509 cert validation chain
  espClient.setInsecure();
#if !defined(ESP32)
  espClient.setBufferSizes(512, 512);
#endif

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(512);

  // Hook command queue state changes to immediately publish retained status
  cmdQueue.setStateChangeCallback([](const DeviceStates& states) {
    if (mqttClient.connected()) {
      mqttClient.publish(MQTT_TOPIC_STATUS, cmdQueue.getStatesJson().c_str(), true);
    }
  });

  Serial.printf("[MQTT] Configured for %s:%d\n", MQTT_HOST, MQTT_PORT);
}

void checkMqttConnection() {
  if (!isMqttConfigured()) return;
  if (WiFi.status() != WL_CONNECTED) return;

  if (mqttClient.connected()) {
    mqttClient.loop();
    return;
  }

  unsigned long now = millis();
  if (now - lastMqttReconnect >= MQTT_RECONNECT_INTERVAL) {
    lastMqttReconnect = now;
    Serial.print(F("[MQTT] Connecting to HiveMQ Cloud... "));

#if defined(ESP32)
    String chipIdStr = String((uint32_t)ESP.getEfuseMac(), HEX);
#else
    String chipIdStr = String(ESP.getChipId(), HEX);
#endif
    String clientId = String(DEVICE_HOSTNAME) + "-" + chipIdStr;

    bool connected = false;
    if (strlen(MQTT_USER) > 0) {
      connected = mqttClient.connect(
        clientId.c_str(),
        MQTT_USER,
        MQTT_PASS,
        MQTT_TOPIC_AVAILABILITY,
        0,
        true,
        "offline"
      );
    } else {
      connected = mqttClient.connect(
        clientId.c_str(),
        MQTT_TOPIC_AVAILABILITY,
        0,
        true,
        "offline"
      );
    }

    if (connected) {
      Serial.println(F("Connected!"));
      mqttClient.publish(MQTT_TOPIC_AVAILABILITY, "online", true);
      mqttClient.publish(MQTT_TOPIC_STATUS, cmdQueue.getStatesJson().c_str(), true);
      mqttClient.subscribe(MQTT_TOPIC_COMMAND);
      mqttClient.subscribe("home/desk_lamp/status");
      Serial.printf("[MQTT] Subscribed to %s and home/desk_lamp/status\n", MQTT_TOPIC_COMMAND);
    } else {
      Serial.printf("Failed (rc=%d). Retrying in %lu s.\n", mqttClient.state(), MQTT_RECONNECT_INTERVAL / 1000);
    }
  }
}

static bool servicesStarted = false;

void startNetworkServices() {
  if (servicesStarted) return;
  servicesStarted = true;

  if (MDNS.begin(DEVICE_HOSTNAME)) {
    MDNS.addService("http", "tcp", HTTP_PORT);
    Serial.print(F("[mDNS] Responder active at http://"));
    Serial.print(F(DEVICE_HOSTNAME));
    Serial.println(F(".local\n"));
  }

  setupOTA();
  setupWebServer();
  setupMqtt();
}

void setupWiFi() {
#if !defined(ESP32)
  WiFi.persistent(false);
#endif
  WiFi.mode(WIFI_STA);
#if defined(ESP32)
  WiFi.setHostname(DEVICE_HOSTNAME);
#else
  WiFi.hostname(DEVICE_HOSTNAME);
#endif
  WiFi.setAutoReconnect(true);

  Serial.print(F("[WiFi] Connecting to SSID: \""));
  Serial.print(F(WIFI_SSID));
  Serial.println(F("\""));

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Allow up to 20 seconds for WPA2 handshake and DHCP lease
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 20000) {
    delay(400);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F("[WiFi] Connected! IP Address: "));
    Serial.println(WiFi.localIP());
    startNetworkServices();
  } else {
    Serial.printf("[WiFi] Still negotiating connection in background (Status code: %d).\n", WiFi.status());
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

void handleSerial() {
  while (Serial.available() > 0) {
    char key = (char)Serial.read();

    if (key == '\r' || key == '\n' || key == ' ' || key == '\0') {
      continue;
    }

    if (key == '?') {
      printMenu();
      continue;
    }

    String keyStr = String(key);
    cmdQueue.dispatchAction(keyStr);
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  // Initialize hardware pins
  cmdQueue.begin();

#if defined(ESP32)
  // Initialize BLE Stack & set state sync callback
  bleManager.begin();
  bleManager.setStateCallback([](const char* device, bool isOn) {
    cmdQueue.setDeviceState(device, isOn);
  });
#else
  // Turn OFF all onboard LEDs on ESP8266 (GPIO 2 / D4 and GPIO 16 / D0 are active-LOW)
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  pinMode(16, OUTPUT);
  digitalWrite(16, HIGH);
#endif

  // Connect to Wi-Fi
  setupWiFi();

  // Print menu
  printMenu();
}

void loop() {
  // Non-blocking hardware pulse and macro state machine
  cmdQueue.update();

#if defined(ESP32)
  // BLE background maintenance & auto-reconnect
  bleManager.loop();
#endif

  // Non-blocking HiveMQ Cloud MQTT maintenance & background reconnect
  checkMqttConnection();

  // Handle incoming HTTP web client requests
  server.handleClient();

#if !defined(ESP32)
  // Handle mDNS queries on ESP8266
  MDNS.update();
#endif

  // Handle OTA update packets
  ArduinoOTA.handle();

  // Check Serial commands from terminal
  handleSerial();

  // Background Wi-Fi health check
  checkWiFiConnection();
}
