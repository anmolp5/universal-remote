#pragma once

#include <Arduino.h>

// ==========================================
// Pin Definitions (ESP8266 Safe GPIOs)
// ==========================================
const uint16_t IR_SEND_PIN    = 14; // D5 / GPIO14 (IR Transmitter LED)

// ESP8266 Pins driving LM3046 Transistor Bases for RF Remote
const uint8_t RF_PIN_POWER    = 12; // D6 / GPIO12
const uint8_t RF_PIN_WARMER   = 5;  // D1 / GPIO5
const uint8_t RF_PIN_DIMMER   = 13; // D7 / GPIO13
const uint8_t RF_PIN_COOLER   = 15; // D8 / GPIO15 (Moved from D0 to disable onboard LED)
const uint8_t RF_PIN_BRIGHTER = 4;  // D2 / GPIO4

// ==========================================
// Timing Constants (Milliseconds) - Tuned for Instant Multi-Device Response
const unsigned long RF_PRESS_DURATION_MS = 100;
const unsigned long RF_COOLDOWN_MS       = 30;
const unsigned long MACRO_STEP_DELAY_MS  = 35;

// ==========================================
// Network & Device Identity
// ==========================================
#define DEVICE_HOSTNAME "universal-remote"
#define HTTP_PORT 80

// ==========================================
// IR Protocol Constants
// ==========================================
// Poster Light (NEC)
const uint16_t POSTER_ADDR         = 0x7386;
const uint8_t  POSTER_CMD_ON       = 0x03;
const uint8_t  POSTER_CMD_OFF      = 0x98;
const uint8_t  POSTER_CMD_COLOR    = 0x50;

// Donut Lamp (NEC)
const uint16_t DONUT_ADDR          = 0x0000;
const uint8_t  DONUT_CMD_ON        = 0x40;
const uint8_t  DONUT_CMD_OFF       = 0x41;

// Bedside Lamp (Custom 10-bit Pulse Distance Protocol)
const uint32_t BEDSIDE_CMD_POWER    = 0x1F2; // 0b01 1111 0010
const uint32_t BEDSIDE_CMD_BRIGHTER = 0x3B2; // 0b11 1011 0010 (Verified working)
const uint32_t BEDSIDE_CMD_DIMMER   = 0x172; // 0b01 0111 0010 (Corrected from 0x1E2)
const uint32_t BEDSIDE_CMD_WARMER   = 0x0E2; // 0b00 1110 0010
const uint32_t BEDSIDE_CMD_COOLER   = 0x3D2; // 0b11 1101 0010

// Bedside Lamp Captured Raw 191-Element Timing Array from Physical Remote (µs)
const uint16_t RAW_BEDSIDE_DIMMER[191] = {
  1180,470, 1180,470, 380,1270, 1180,470, 1180,520, 1180,470, 330,1320, 330,1320, 380,1320, 330,1270, 1230,470, 1180,7220,
  1180,470, 1180,470, 380,1270, 1230,420, 1230,470, 1180,470, 330,1320, 380,1270, 380,1270, 380,1270, 1230,470, 1180,7220,
  1180,470, 1180,470, 380,1270, 1230,470, 1180,470, 1180,470, 380,1270, 380,1270, 380,1270, 380,1320, 1180,470, 1180,7220,
  1180,470, 1230,420, 380,1320, 1180,470, 1180,470, 1180,520, 330,1320, 330,1320, 330,1320, 330,1320, 1180,470, 1180,7270,
  1180,470, 1180,470, 330,1320, 1180,520, 1130,520, 1130,520, 330,1320, 330,1320, 330,1320, 380,1270, 1180,520, 1130,7270,
  1180,470, 1180,520, 330,1320, 1130,520, 1180,470, 1180,470, 330,1320, 330,1320, 380,1320, 330,1320, 1130,520, 1180,7220,
  1180,520, 1130,520, 330,1320, 1180,470, 1180,470, 1180,470, 380,1320, 330,1320, 330,1320, 330,1320, 1180,470, 1180,7270,
  1130,520, 1180,470, 330,1320, 1180,520, 1130,520, 1180,470, 330,1370, 330,1270, 330,1320, 330,1370, 1130,520, 1130
};
const uint16_t RAW_BEDSIDE_DIMMER_LEN = 191;

const uint16_t RAW_BEDSIDE_BRIGHTER[23] = {
  1300, 400,   // Header
  1250, 400,   // Bit 0: '0'
  400, 1250,   // Bit 1: '1'
  1250, 400,   // Bit 2: '0'
  1250, 400,   // Bit 3: '0'
  400, 1250,   // Bit 4: '1'
  400, 1250,   // Bit 5: '1'
  1250, 400,   // Bit 6: '0'
  400, 1250,   // Bit 7: '1'
  400, 1250,   // Bit 8: '1'
  400, 1250,   // Bit 9: '1'
  400          // Stop Mark
};

// =============================================================================
// MQTT Topic Definitions
// =============================================================================
#ifndef MQTT_TOPIC_COMMAND
#define MQTT_TOPIC_COMMAND      MQTT_TOPIC_PREFIX "/command"
#endif

#ifndef MQTT_TOPIC_STATUS
#define MQTT_TOPIC_STATUS       MQTT_TOPIC_PREFIX "/status"
#endif

#ifndef MQTT_TOPIC_AVAILABILITY
#define MQTT_TOPIC_AVAILABILITY MQTT_TOPIC_PREFIX "/availability"
#endif

