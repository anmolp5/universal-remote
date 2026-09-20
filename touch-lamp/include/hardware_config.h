#pragma once

#include <Arduino.h>

// =============================================================================
// Pin Definitions (ESP8266 Safe GPIOs)
// =============================================================================
// Touch Control: D2 (GPIO 4) drives NPN transistor base via 1k resistor (10k pull-down).
// Collector connects to touch IC sense pad R5, emitter to GND.
// NOTE: D2 / GPIO 4 is completely free of boot-strapping restrictions, unlike D4/GPIO2.
const uint8_t PIN_TOUCH_CTRL  = 4; // D2 / GPIO4

// Closed-loop State Sensing: D5 (GPIO 14) reads lamp standby/night indicator LED anode.
// When Lamp is OFF: Standby LED is lit (measured 2.6V) -> digitalRead(14) == HIGH.
// When Lamp is ON:  Standby LED is dark (0V)           -> digitalRead(14) == LOW.
const uint8_t PIN_STATE_SENSE = 14; // D5 / GPIO14

// =============================================================================
// Timing Constants (Milliseconds)
// =============================================================================
const unsigned long PULSE_TAP_MS      = 180; // Duration of active touch pulse
const unsigned long INTER_TAP_GAP_MS  = 250; // Delay between consecutive taps
const unsigned long SENSE_DEBOUNCE_MS = 50;  // Debounce filter for physical touch sensing

// =============================================================================
// Network & Device Identity
// =============================================================================
#define DEVICE_HOSTNAME "desk-lamp"
#define HTTP_PORT 80

// =============================================================================
// MQTT Topic Definitions
// =============================================================================
#define MQTT_TOPIC_COMMAND      "home/desk_lamp/command"
#define MQTT_TOPIC_STATUS       "home/desk_lamp/status"
#define MQTT_TOPIC_AVAILABILITY "home/desk_lamp/availability"
#define MQTT_TOPIC_MASTER_CMD   "home/universal_remote/command"
