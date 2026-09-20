#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include "secrets.h"
#include "hardware_config.h"
#include "command_queue.h"
#include "web_ui.h"

// HTTP Web Server on port 80
ESP8266WebServer server(HTTP_PORT);

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
  Serial.println(F("   Universal IR & RF Lighting Hub (ESP8266) Ready    "));
  Serial.println(F("====================================================="));
  Serial.println(F("Poster Light (IR NEC):"));
  Serial.println(F("  1 : ON | 2 : OFF | 3 : Color"));
  Serial.println(F("Donut Lamp (IR NEC):"));
  Serial.println(F("  4 : ON | 5 : OFF"));
  Serial.println(F("Bedside Lamp (IR Pulse Distance):"));
  Serial.println(F("  6 : Power | 7 : Brighter | 8 : Dimmer | 9 : Warmer | 0 : Cooler"));
  Serial.println(F("Floor Lamp (RF):"));
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
  } else {
    Serial.print(F("Wi-Fi Status [Code "));
    Serial.print(WiFi.status());
    Serial.print(F("]: "));
    switch (WiFi.status()) {
      case WL_IDLE_STATUS: Serial.println(F("Idle")); break;
      case WL_NO_SSID_AVAIL: Serial.println(F("SSID Not Found / Out of range")); break;
      case WL_SCAN_COMPLETED: Serial.println(F("Scan Completed")); break;
      case WL_CONNECT_FAILED: Serial.println(F("Authentication / Password Failed")); break;
      case WL_CONNECTION_LOST: Serial.println(F("Connection Lost")); break;
      case WL_DISCONNECTED: Serial.println(F("Connecting / Disconnected")); break;
      default: Serial.println(F("Unknown")); break;
    }
  }
  Serial.println(F("=====================================================\n"));
}

void setupWebServer() {
  // Serve the Web UI from flash memory (PROGMEM)
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", INDEX_HTML);
  });

  // REST API: Trigger a command
  server.on("/api/command", HTTP_POST, []() {
    if (!server.hasArg("action")) {
      server.send(400, "application/json", F("{\"error\":\"Missing action parameter\"}"));
      return;
    }

    String action = server.arg("action");
    bool dispatched = cmdQueue.dispatchAction(action);

    if (dispatched) {
      String res = F("{\"success\":true,\"action\":\"") + action + F("\",\"states\":") + cmdQueue.getStatesJson() + F("}");
      server.send(200, "application/json", res);
    } else {
      server.send(404, "application/json", F("{\"error\":\"Unknown action\"}"));
    }
  });

  // REST API: Query current assumed device states
  server.on("/api/status", HTTP_GET, []() {
    server.send(200, "application/json", cmdQueue.getStatesJson());
  });

  // REST API: Manual state resynchronization
  server.on("/api/resync", HTTP_POST, []() {
    if (!server.hasArg("device") || !server.hasArg("state")) {
      server.send(400, "application/json", F("{\"error\":\"Missing device or state parameter\"}"));
      return;
    }

    String device = server.arg("device");
    bool state = (server.arg("state").toInt() == 1);
    cmdQueue.setDeviceState(device, state);

    String res = F("{\"success\":true,\"states\":") + cmdQueue.getStatesJson() + F("}");
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

  // REST API: Directly pulse any individual GPIO pin for hardware diagnosis
  server.on("/api/pulse_pin", HTTP_ANY, []() {
    if (!server.hasArg("pin")) {
      server.send(400, "application/json", F("{\"error\":\"Missing pin parameter\"}"));
      return;
    }
    uint8_t pin = server.arg("pin").toInt();
    cmdQueue.enqueueRFPulse(pin, "Direct GPIO Pulse");
    Serial.printf("[Diagnostic] Pulsing GPIO %d directly\n", pin);
    String res = F("{\"success\":true,\"pulsed_pin\":") + String(pin) + F("}");
    server.send(200, "application/json", res);
  });

  // REST API: Inspect all GPIO pin levels
  server.on("/api/pins", HTTP_GET, []() {
    uint8_t pins[] = { 16, 5, 4, 12, 13, 14, 0, 2, 15 };
    String res = "{";
    for (size_t i = 0; i < sizeof(pins); i++) {
      if (i > 0) res += ",";
      res += "\"GPIO" + String(pins[i]) + "\":" + String(digitalRead(pins[i]));
    }
    res += "}";
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
  espClient.setBufferSizes(512, 512);

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

    String clientId = String(DEVICE_HOSTNAME) + "-" + String(ESP.getChipId(), HEX);

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
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(DEVICE_HOSTNAME);
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

    // Ignore line endings / whitespace
    if (key == '\r' || key == '\n' || key == ' ' || key == '\0') {
      continue;
    }

    if (key == '?') {
      printMenu();
      continue;
    }

    // Pass single key character to command queue
    String keyStr = String(key);
    cmdQueue.dispatchAction(keyStr);
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  // Initialize IR and RF hardware pins
  cmdQueue.begin();

  // Turn OFF all onboard LEDs (GPIO 2 / D4 and GPIO 16 / D0 are active-LOW)
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  pinMode(16, OUTPUT);
  digitalWrite(16, HIGH);

  // Connect to Wi-Fi
  setupWiFi();

  // Print menu
  printMenu();
}

void loop() {
  // Non-blocking hardware pulse and macro state machine
  cmdQueue.update();

  // Non-blocking HiveMQ Cloud MQTT maintenance & background reconnect
  checkMqttConnection();

  // Handle incoming HTTP web client requests
  server.handleClient();

  // Handle mDNS queries
  MDNS.update();

  // Handle OTA update packets
  ArduinoOTA.handle();

  // Check Serial commands from terminal
  handleSerial();

  // Background Wi-Fi health check
  checkWiFiConnection();
}
