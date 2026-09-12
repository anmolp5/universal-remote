#pragma once

// =============================================================================
// Wi-Fi & Network Credentials Template
// Copy this file to include/secrets.h and fill in your actual credentials.
// =============================================================================

#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Optional ArduinoOTA password for wireless flashing
#define OTA_PASSWORD "admin123"

// =============================================================================
// HiveMQ Cloud / Remote MQTT Broker Configuration
// =============================================================================
// HiveMQ Cloud cluster URL (e.g., "your-cluster-id.s1.eu.hivemq.cloud")
#define MQTT_HOST "YOUR_CLUSTER_ID.s1.eu.hivemq.cloud"
#define MQTT_PORT 8883
#define MQTT_USER "YOUR_MQTT_USERNAME"
#define MQTT_PASS "YOUR_MQTT_PASSWORD"

// Base topic prefix for all pub/sub topics
#define MQTT_TOPIC_PREFIX "home/universal_remote"
