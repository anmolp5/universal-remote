#pragma once

#include <Arduino.h>

#if defined(ESP32)
#include <NimBLEDevice.h>
#include "hardware_config.h"

typedef void (*BleStateCallback)(const char* device, bool isOn);

enum LightMode {
  MODE_WHITE_CCT,
  MODE_RGB
};

class BleManager {
public:
  BleManager();
  void begin();
  void loop();

  // Control Floor Lamp (MELK-OA10WCT)
  bool setFloorPower(bool on);
  bool setFloorBrightness(uint8_t percent);
  bool setFloorColor(uint8_t r, uint8_t g, uint8_t b);
  bool setFloorCCT(uint8_t warm, uint8_t cool);
  bool adjustFloorBrightness(bool brighter);
  bool adjustFloorTemp(bool warmer);

  // Control Donut Lamp (ELK-BLEDOM07)
  bool setDonutPower(bool on);
  bool setDonutBrightness(uint8_t percent);
  bool setDonutColor(uint8_t r, uint8_t g, uint8_t b);

  // Hybrid Management
  void releaseBle();
  void reconnectBle();

  // Active State Lock Enforcement (Floor Lamp only)
  void enforceFloorState();
  void handleBleNotification(NimBLERemoteCharacteristic* pChar, uint8_t* data, size_t len);

  // State inspection
  bool isFloorConnected();
  bool isDonutConnected();
  bool isReleased() const { return userReleased; }
  uint8_t getFloorBrightness() const { return floorBrightness; }
  uint8_t getDonutBrightness() const { return donutBrightness; }

  void setStateCallback(BleStateCallback cb) { stateCallback = cb; }

private:
  bool connectDevice(const char* mac, NimBLEClient*& pClient, NimBLERemoteCharacteristic*& pChar, NimBLERemoteCharacteristic*& pNotifyChar, const char* label);
  bool ensureFloorConnected();
  bool ensureDonutConnected();
  bool sendRaw(NimBLEClient* pClient, NimBLERemoteCharacteristic* pChar, const uint8_t* data, size_t len);

  void loadSettingsFromNVS();
  void saveSettingsToNVS();

  NimBLEClient* pFloorClient;
  NimBLERemoteCharacteristic* pFloorChar;
  NimBLERemoteCharacteristic* pFloorNotifyChar;

  NimBLEClient* pDonutClient;
  NimBLERemoteCharacteristic* pDonutChar;

  bool userReleased;
  bool isEnforcing;
  unsigned long lastFloorRetry;
  unsigned long lastDonutRetry;
  unsigned long lastCommandTime;

  bool floorTargetOn;
  LightMode floorMode;
  uint8_t floorWarm;
  uint8_t floorCool;
  uint8_t floorR, floorG, floorB;
  uint8_t floorBrightness;
  int8_t floorTempStep; // -3 (cool) to +3 (warm)

  uint8_t donutR, donutG, donutB;
  uint8_t donutBrightness;

  BleStateCallback stateCallback;
};

extern BleManager bleManager;

#endif // ESP32
