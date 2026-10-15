#include "ble_manager.h"
#include <Preferences.h>

#if defined(ESP32)

BleManager bleManager;

BleManager::BleManager()
  : pFloorClient(nullptr), pFloorChar(nullptr), pFloorNotifyChar(nullptr),
    pDonutClient(nullptr), pDonutChar(nullptr),
    pCeilingClient(nullptr), pCeilingChar(nullptr),
    userReleased(false), isEnforcing(false),
    lastFloorRetry(0), lastDonutRetry(0), lastCeilingRetry(0),
    lastCommandTime(0),
    floorTargetOn(false), floorMode(MODE_WHITE_CCT),
    floorWarm(50), floorCool(50),
    floorR(255), floorG(165), floorB(60),
    floorBrightness(100), floorTempStep(0),
    donutR(255), donutG(165), donutB(60),
    donutBrightness(100),
    ceilingMac(CEILING_LIGHT_BLE_ADDR),
    ceilingR(CEILING_RGB_NEUTRAL_R), ceilingG(CEILING_RGB_NEUTRAL_G), ceilingB(CEILING_RGB_NEUTRAL_B),
    ceilingBrightness(100), ceilingTempStep(0),
    stateCallback(nullptr) {}

void BleManager::loadSettingsFromNVS() {
  Preferences p;
  p.begin("ble_lamps", true);
  floorTargetOn   = p.getBool("floor_on", false);
  floorMode       = (LightMode)p.getUChar("floor_mode", (uint8_t)MODE_WHITE_CCT);
  floorWarm       = p.getUChar("floor_warm", 50);
  floorCool       = p.getUChar("floor_cool", 50);
  floorR          = p.getUChar("floor_r", 255);
  floorG          = p.getUChar("floor_g", 165);
  floorB          = p.getUChar("floor_b", 60);
  floorBrightness = p.getUChar("floor_bri", 100);
  floorTempStep   = p.getChar("floor_step", 0);

  donutR          = p.getUChar("donut_r", 255);
  donutG          = p.getUChar("donut_g", 165);
  donutB          = p.getUChar("donut_b", 60);
  donutBrightness = p.getUChar("donut_bri", 100);

  ceilingMac        = p.getString("ceil_mac", CEILING_LIGHT_BLE_ADDR);
  ceilingR          = p.getUChar("ceil_r", CEILING_RGB_NEUTRAL_R);
  ceilingG          = p.getUChar("ceil_g", CEILING_RGB_NEUTRAL_G);
  ceilingB          = p.getUChar("ceil_b", CEILING_RGB_NEUTRAL_B);
  ceilingBrightness = p.getUChar("ceil_bri", 100);
  ceilingTempStep   = p.getChar("ceil_step", 0);
  p.end();

  Serial.printf("[NVS] BLE settings restored: FloorOn=%d, Warm=%d, Bri=%d | CeilMac='%s', Bri=%d\n",
    floorTargetOn, floorWarm, floorBrightness, ceilingMac.c_str(), ceilingBrightness);
}

void BleManager::saveSettingsToNVS() {
  Preferences p;
  p.begin("ble_lamps", false);
  p.putBool("floor_on", floorTargetOn);
  p.putUChar("floor_mode", (uint8_t)floorMode);
  p.putUChar("floor_warm", floorWarm);
  p.putUChar("floor_cool", floorCool);
  p.putUChar("floor_r", floorR);
  p.putUChar("floor_g", floorG);
  p.putUChar("floor_b", floorB);
  p.putUChar("floor_bri", floorBrightness);
  p.putChar("floor_step", floorTempStep);

  p.putUChar("donut_r", donutR);
  p.putUChar("donut_g", donutG);
  p.putUChar("donut_b", donutB);
  p.putUChar("donut_bri", donutBrightness);

  p.putString("ceil_mac", ceilingMac);
  p.putUChar("ceil_r", ceilingR);
  p.putUChar("ceil_g", ceilingG);
  p.putUChar("ceil_b", ceilingB);
  p.putUChar("ceil_bri", ceilingBrightness);
  p.putChar("ceil_step", ceilingTempStep);
  p.end();
}

void BleManager::begin() {
  loadSettingsFromNVS();

  NimBLEDevice::init("Universal-Remote-BLE");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9); // Max TX power for robust range
  NimBLEDevice::setSecurityAuth(false, false, false);
  Serial.println(F("[BLE] NimBLE stack initialized."));

  // Connect to devices initially
  ensureFloorConnected();
  ensureDonutConnected();
  ensureCeilingConnected();
}

void BleManager::loop() {
  if (userReleased) return;

  unsigned long now = millis();

  // Background retry for Floor Lamp every 15 seconds if disconnected
  if (!isFloorConnected() && (now - lastFloorRetry > 15000)) {
    lastFloorRetry = now;
    ensureFloorConnected();
  }

  // Background retry for Donut Lamp every 15 seconds if disconnected
  if (!isDonutConnected() && (now - lastDonutRetry > 15000)) {
    lastDonutRetry = now;
    ensureDonutConnected();
  }

  // Background retry for Ceiling Lights every 15 seconds if disconnected
  if (ceilingMac.length() > 0 && !isCeilingConnected() && (now - lastCeilingRetry > 15000)) {
    lastCeilingRetry = now;
    ensureCeilingConnected();
  }
}

bool BleManager::connectDevice(const char* mac, NimBLEClient*& pClient, NimBLERemoteCharacteristic*& pChar, NimBLERemoteCharacteristic*& pNotifyChar, const char* label) {
  if (pClient && pClient->isConnected() && pChar != nullptr) {
    return true;
  }

  if (!pClient) {
    pClient = NimBLEDevice::createClient();
    pClient->setConnectTimeout(4); // 4 seconds timeout
  }

  if (!pClient->isConnected()) {
    Serial.printf("[BLE] Connecting to %s (%s)...\n", label, mac);

    bool connected = false;
    // Try Random address first (standard for modern Telink LED controllers)
    NimBLEAddress addrRandom(mac, BLE_ADDR_RANDOM);
    if (pClient->connect(addrRandom, false)) {
      connected = true;
    } else {
      // Fallback to Public address
      NimBLEAddress addrPublic(mac, BLE_ADDR_PUBLIC);
      if (pClient->connect(addrPublic, false)) {
        connected = true;
      }
    }

    if (!connected) {
      Serial.printf("[BLE] Failed to connect to %s.\n", label);
      return false;
    }
  }

  Serial.printf("[BLE] Connected to %s! Discovering characteristics...\n", label);
  pChar = nullptr;
  pNotifyChar = nullptr;

  // First check known ELK/Lotus service 0xFFF0 -> 0xFFF3 (write) & 0xFFF4 (notify)
  NimBLERemoteService* pTargetService = pClient->getService("fff0");
  if (!pTargetService) pTargetService = pClient->getService("FFF0");
  if (pTargetService) {
    pChar = pTargetService->getCharacteristic("fff3");
    if (!pChar) pChar = pTargetService->getCharacteristic("FFF3");
    pNotifyChar = pTargetService->getCharacteristic("fff4");
    if (!pNotifyChar) pNotifyChar = pTargetService->getCharacteristic("FFF4");
  }

  // Second check known BLE service 0xFFE0 -> 0xFFE1 (write) & 0xFFE2 (notify)
  if (!pChar) {
    pTargetService = pClient->getService("ffe0");
    if (!pTargetService) pTargetService = pClient->getService("FFE0");
    if (pTargetService) {
      pChar = pTargetService->getCharacteristic("ffe1");
      if (!pChar) pChar = pTargetService->getCharacteristic("FFE1");
      pNotifyChar = pTargetService->getCharacteristic("ffe2");
      if (!pNotifyChar) pNotifyChar = pTargetService->getCharacteristic("FFE2");
    }
  }

  // Fallback: Discover all characteristics and find writable/notify
  if (!pChar) {
    std::vector<NimBLERemoteService*>* pServices = pClient->getServices(true);
    if (pServices) {
      for (auto* pService : *pServices) {
        std::vector<NimBLERemoteCharacteristic*>* pChars = pService->getCharacteristics(true);
        if (!pChars) continue;

        for (auto* pC : *pChars) {
          if (pC->canWrite() || pC->canWriteNoResponse()) {
            String uuidStr = pC->getUUID().toString().c_str();
            uuidStr.toLowerCase();
            if (uuidStr.indexOf("fff3") >= 0) {
              pChar = pC;
            } else if (!pChar) {
              pChar = pC;
            }
          }
          if (pC->canNotify() || pC->canIndicate()) {
            pNotifyChar = pC;
          }
        }
      }
    }
  }

  if (pChar) {
    Serial.printf("[BLE] Ready to send commands for %s (Char: %s)\n", label, pChar->getUUID().toString().c_str());
    if (pNotifyChar && strstr(label, "Floor") != nullptr && (pNotifyChar->canNotify() || pNotifyChar->canIndicate())) {
      pNotifyChar->subscribe(true, [](NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
        bleManager.handleBleNotification(pBLERemoteCharacteristic, pData, length);
      });
      Serial.printf("[BLE] Subscribed to notifications on %s (Char: %s)\n", label, pNotifyChar->getUUID().toString().c_str());
    }
    return true;
  } else {
    Serial.printf("[BLE] No writable characteristic found on %s.\n", label);
    pClient->disconnect();
    return false;
  }
}

bool BleManager::ensureFloorConnected() {
  if (userReleased) return false;
  return connectDevice(FLOOR_LAMP_BLE_ADDR, pFloorClient, pFloorChar, pFloorNotifyChar, "Floor Lamp (Lotus)");
}

bool BleManager::ensureDonutConnected() {
  if (userReleased) return false;
  NimBLERemoteCharacteristic* dummyNotify = nullptr;
  return connectDevice(DONUT_LAMP_BLE_ADDR, pDonutClient, pDonutChar, dummyNotify, "Donut Lamp");
}

bool BleManager::ensureCeilingConnected() {
  if (userReleased || ceilingMac.length() == 0) return false;
  NimBLERemoteCharacteristic* dummyNotify = nullptr;
  return connectDevice(ceilingMac.c_str(), pCeilingClient, pCeilingChar, dummyNotify, "Ceiling Lights");
}

bool BleManager::sendRaw(NimBLEClient* pClient, NimBLERemoteCharacteristic* pChar, const uint8_t* data, size_t len) {
  if (!pClient || !pClient->isConnected() || !pChar) {
    Serial.println(F("[BLE Error] sendRaw: device not connected or characteristic null!"));
    return false;
  }

  Serial.print(F("[BLE TX] Sending: "));
  for (size_t i = 0; i < len; i++) {
    if (data[i] < 0x10) Serial.print('0');
    Serial.print(data[i], HEX);
    Serial.print(' ');
  }
  Serial.println();

  bool ok = pChar->writeValue(data, len, !pChar->canWriteNoResponse());
  Serial.printf("[BLE TX] Result: %s\n", ok ? "SUCCESS" : "FAILED");
  return ok;
}

// -----------------------------------------------------------------------------
// Floor Lamp (Lotus Lamp MELK-OA10WCT) Control
// -----------------------------------------------------------------------------
bool BleManager::setFloorPower(bool on) {
  userReleased = false; // Reclaim connection if user triggers command
  if (!ensureFloorConnected()) return false;
  lastCommandTime = millis();
  floorTargetOn = on;

  bool ok = false;
  if (on) {
    // Send target CCT or RGB directly — lights up immediately in target state with NO flashing and NO green tint
    if (floorMode == MODE_WHITE_CCT) {
      uint8_t cctPacket[] = { 0x7E, 0x06, 0x05, 0x02, floorWarm, floorCool, 0xFF, 0x08, 0xEF };
      ok = sendRaw(pFloorClient, pFloorChar, cctPacket, sizeof(cctPacket));
    } else {
      uint8_t rgbPacket[] = { 0x7E, 0x07, 0x05, 0x03, floorR, floorG, floorB, 0x10, 0xEF };
      ok = sendRaw(pFloorClient, pFloorChar, rgbPacket, sizeof(rgbPacket));
    }
  } else {
    // Native MELK-OA10 Power OFF: 7e 04 04 00 00 00 ff 00 ef
    const uint8_t p1[] = { 0x7E, 0x04, 0x04, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xEF };
    ok = sendRaw(pFloorClient, pFloorChar, p1, sizeof(p1));
    delay(40);
    // Standard Lotus Power OFF fallback: 7e 00 04 00 00 00 ff 00 ef
    const uint8_t p2[] = { 0x7E, 0x00, 0x04, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xEF };
    sendRaw(pFloorClient, pFloorChar, p2, sizeof(p2));
  }

  saveSettingsToNVS();

  if (stateCallback) {
    stateCallback("floor", on);
  }
  return ok;
}

bool BleManager::setFloorBrightness(uint8_t percent) {
  userReleased = false;
  if (!ensureFloorConnected()) return false;
  lastCommandTime = millis();
  if (percent > 100) percent = 100;
  if (percent < 5) percent = 5;
  floorBrightness = percent;
  saveSettingsToNVS();

  bool ok = false;
  if (floorMode == MODE_WHITE_CCT) {
    // In CCT mode, scale warm and cool diode values by floorBrightness
    uint8_t scaledWarm = (uint16_t)floorWarm * percent / 100;
    uint8_t scaledCool = (uint16_t)floorCool * percent / 100;
    uint8_t cctPacket[] = { 0x7E, 0x06, 0x05, 0x02, scaledWarm, scaledCool, 0xFF, 0x08, 0xEF };
    ok = sendRaw(pFloorClient, pFloorChar, cctPacket, sizeof(cctPacket));
    delay(25);
    // Also send native MELK white intensity channel packet: 7e 07 05 01 [val] ff 02 01 ef
    uint8_t whitePacket[] = { 0x7E, 0x07, 0x05, 0x01, percent, 0xFF, 0x02, 0x01, 0xEF };
    sendRaw(pFloorClient, pFloorChar, whitePacket, sizeof(whitePacket));
  } else {
    // In RGB mode, send standard RGB brightness packet
    uint8_t packet[] = { 0x7E, 0x07, 0x01, percent, 0xFF, 0xFF, 0xFF, 0x00, 0xEF };
    ok = sendRaw(pFloorClient, pFloorChar, packet, sizeof(packet));
    delay(25);
    uint8_t elkBri[] = { 0x7E, 0x00, 0x01, percent, 0x00, 0x00, 0x00, 0x00, 0xEF };
    sendRaw(pFloorClient, pFloorChar, elkBri, sizeof(elkBri));
  }
  return ok;
}

bool BleManager::adjustFloorBrightness(bool brighter) {
  int newBri = (int)floorBrightness + (brighter ? 15 : -15);
  if (newBri > 100) newBri = 100;
  if (newBri < 10) newBri = 10;
  Serial.printf("[BLE] adjustFloorBrightness: %s -> %d%%\n", brighter ? "Brighter" : "Dimmer", newBri);
  return setFloorBrightness((uint8_t)newBri);
}

bool BleManager::setFloorColor(uint8_t r, uint8_t g, uint8_t b) {
  userReleased = false;
  if (!ensureFloorConnected()) return false;
  lastCommandTime = millis();

  floorMode = MODE_RGB;
  floorR = r;
  floorG = g;
  floorB = b;
  saveSettingsToNVS();

  // 7e 07 05 03 [R] [G] [B] 10 ef
  uint8_t packet[] = { 0x7E, 0x07, 0x05, 0x03, r, g, b, 0x10, 0xEF };
  return sendRaw(pFloorClient, pFloorChar, packet, sizeof(packet));
}

bool BleManager::setFloorCCT(uint8_t warm, uint8_t cool) {
  userReleased = false;
  if (!ensureFloorConnected()) return false;
  lastCommandTime = millis();

  if (warm > 100) warm = 100;
  if (cool > 100) cool = 100;

  floorMode = MODE_WHITE_CCT;
  floorWarm = warm;
  floorCool = cool;

  // Sync internal temp step to nearest preset
  if (warm >= 90) floorTempStep = 3;
  else if (cool >= 90) floorTempStep = -3;
  else if (abs((int)warm - (int)cool) <= 20) floorTempStep = 0;

  saveSettingsToNVS();

  Serial.printf("[BLE] Floor CCT: Warm=%d%%, Cool=%d%%\n", warm, cool);

  // Single MELK-OA10 native CCT command: 7E 06 05 02 [warm] [cool] FF 08 EF
  uint8_t packet[] = { 0x7E, 0x06, 0x05, 0x02, warm, cool, 0xFF, 0x08, 0xEF };
  return sendRaw(pFloorClient, pFloorChar, packet, sizeof(packet));
}

bool BleManager::adjustFloorTemp(bool warmer) {
  if (warmer) {
    if (floorTempStep < 3) floorTempStep++;
  } else {
    if (floorTempStep > -3) floorTempStep--;
  }

  // Smooth steps between 100% Cool White and 100% Warm White
  uint8_t warm = 50;
  uint8_t cool = 50;
  switch (floorTempStep) {
    case -3: warm = 0;   cool = 100; break; // Coolest (6500K)
    case -2: warm = 17;  cool = 83;  break; // Daylight (5500K)
    case -1: warm = 33;  cool = 67;  break; // Natural (4500K)
    case  0: warm = 50;  cool = 50;  break; // Neutral White (4000K)
    case  1: warm = 67;  cool = 33;  break; // Soft White (3500K)
    case  2: warm = 83;  cool = 17;  break; // Warm White (3000K)
    case  3: warm = 100; cool = 0;   break; // Ultra Warm (2200K)
  }

  Serial.printf("[BLE] adjustFloorTemp: step=%d -> Warm=%d%%, Cool=%d%%\n", floorTempStep, warm, cool);
  return setFloorCCT(warm, cool);
}

// -----------------------------------------------------------------------------
// Donut Lamp (ELK-BLEDOM07) Control
// -----------------------------------------------------------------------------
bool BleManager::setDonutPower(bool on) {
  userReleased = false;
  if (!ensureDonutConnected()) return false;
  lastCommandTime = millis();

  bool ok = false;
  if (on) {
    // 7e 00 04 f0 00 01 ff 00 ef
    const uint8_t p1[] = { 0x7E, 0x00, 0x04, 0xF0, 0x00, 0x01, 0xFF, 0x00, 0xEF };
    ok = sendRaw(pDonutClient, pDonutChar, p1, sizeof(p1));
    delay(40);
    // Restore color
    uint8_t colorPacket[] = { 0x7E, 0x00, 0x05, 0x03, donutR, donutG, donutB, 0x00, 0xEF };
    sendRaw(pDonutClient, pDonutChar, colorPacket, sizeof(colorPacket));
    delay(30);
    // Restore brightness
    uint8_t briPacket[] = { 0x7E, 0x00, 0x01, donutBrightness, 0x00, 0x00, 0x00, 0x00, 0xEF };
    sendRaw(pDonutClient, pDonutChar, briPacket, sizeof(briPacket));
  } else {
    // 7e 00 04 00 00 00 ff 00 ef
    const uint8_t p1[] = { 0x7E, 0x00, 0x04, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xEF };
    ok = sendRaw(pDonutClient, pDonutChar, p1, sizeof(p1));
  }

  saveSettingsToNVS();

  if (stateCallback) {
    stateCallback("donut", on);
  }
  return ok;
}

bool BleManager::setDonutBrightness(uint8_t percent) {
  userReleased = false;
  if (!ensureDonutConnected()) return false;
  lastCommandTime = millis();
  if (percent > 100) percent = 100;
  donutBrightness = percent;
  saveSettingsToNVS();

  // 7e 00 01 [val] 00 00 00 00 ef
  uint8_t packet1[] = { 0x7E, 0x00, 0x01, percent, 0x00, 0x00, 0x00, 0x00, 0xEF };
  return sendRaw(pDonutClient, pDonutChar, packet1, sizeof(packet1));
}

bool BleManager::setDonutColor(uint8_t r, uint8_t g, uint8_t b) {
  userReleased = false;
  if (!ensureDonutConnected()) return false;
  lastCommandTime = millis();

  donutR = r;
  donutG = g;
  donutB = b;
  saveSettingsToNVS();

  // 7e 00 05 03 [R] [G] [B] 00 ef
  uint8_t packet1[] = { 0x7E, 0x00, 0x05, 0x03, r, g, b, 0x00, 0xEF };
  return sendRaw(pDonutClient, pDonutChar, packet1, sizeof(packet1));
}

// -----------------------------------------------------------------------------
// Ceiling Lights (ELK-BLEDOM) Control
// -----------------------------------------------------------------------------
bool BleManager::setCeilingPower(bool on) {
  userReleased = false;
  if (!ensureCeilingConnected()) return false;
  lastCommandTime = millis();

  bool ok = false;
  if (on) {
    // ELK-BLEDOM Turn ON: 7e 00 04 f0 00 01 ff 00 ef
    const uint8_t p1[] = { 0x7E, 0x00, 0x04, 0xF0, 0x00, 0x01, 0xFF, 0x00, 0xEF };
    ok = sendRaw(pCeilingClient, pCeilingChar, p1, sizeof(p1));
    delay(40);
    // Restore current RGB color
    uint8_t colorPacket[] = { 0x7E, 0x00, 0x05, 0x03, ceilingR, ceilingG, ceilingB, 0x00, 0xEF };
    sendRaw(pCeilingClient, pCeilingChar, colorPacket, sizeof(colorPacket));
    delay(30);
    // Restore current brightness
    uint8_t briPacket[] = { 0x7E, 0x00, 0x01, ceilingBrightness, 0x00, 0x00, 0x00, 0x00, 0xEF };
    sendRaw(pCeilingClient, pCeilingChar, briPacket, sizeof(briPacket));
  } else {
    // ELK-BLEDOM Turn OFF: 7e 00 04 00 00 00 ff 00 ef
    const uint8_t p1[] = { 0x7E, 0x00, 0x04, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xEF };
    ok = sendRaw(pCeilingClient, pCeilingChar, p1, sizeof(p1));
  }

  saveSettingsToNVS();

  if (stateCallback) {
    stateCallback("ceiling", on);
  }
  return ok;
}

bool BleManager::setCeilingBrightness(uint8_t percent) {
  userReleased = false;
  if (!ensureCeilingConnected()) return false;
  lastCommandTime = millis();
  if (percent > 100) percent = 100;
  if (percent < 5) percent = 5;
  ceilingBrightness = percent;
  saveSettingsToNVS();

  // 7e 00 01 [val] 00 00 00 00 ef
  uint8_t packet1[] = { 0x7E, 0x00, 0x01, percent, 0x00, 0x00, 0x00, 0x00, 0xEF };
  return sendRaw(pCeilingClient, pCeilingChar, packet1, sizeof(packet1));
}

bool BleManager::adjustCeilingBrightness(bool brighter) {
  int newBri = (int)ceilingBrightness + (brighter ? 15 : -15);
  if (newBri > 100) newBri = 100;
  if (newBri < 10) newBri = 10;
  Serial.printf("[BLE] adjustCeilingBrightness: %s -> %d%%\n", brighter ? "Brighter" : "Dimmer", newBri);
  return setCeilingBrightness((uint8_t)newBri);
}

bool BleManager::setCeilingColor(uint8_t r, uint8_t g, uint8_t b) {
  userReleased = false;
  if (!ensureCeilingConnected()) return false;
  lastCommandTime = millis();

  ceilingR = r;
  ceilingG = g;
  ceilingB = b;
  saveSettingsToNVS();

  // 7e 00 05 03 [R] [G] [B] 00 ef
  uint8_t packet1[] = { 0x7E, 0x00, 0x05, 0x03, r, g, b, 0x00, 0xEF };
  return sendRaw(pCeilingClient, pCeilingChar, packet1, sizeof(packet1));
}

bool BleManager::adjustCeilingTemp(bool warmer) {
  if (warmer) {
    if (ceilingTempStep < 2) ceilingTempStep++;
  } else {
    if (ceilingTempStep > -2) ceilingTempStep--;
  }

  uint8_t r = CEILING_RGB_NEUTRAL_R;
  uint8_t g = CEILING_RGB_NEUTRAL_G;
  uint8_t b = CEILING_RGB_NEUTRAL_B;

  switch (ceilingTempStep) {
    case -2: r = 200; g = 225; b = 255; break; // Coolest (Ice Blue)
    case -1: r = 210; g = 230; b = 255; break; // Cool White (CEILING_RGB_COOL)
    case  0: r = 255; g = 235; b = 210; break; // Neutral White (CEILING_RGB_NEUTRAL)
    case  1: r = 255; g = 195; b = 120; break; // Warm White (Soft Amber)
    case  2: r = 255; g = 165; b =  60; break; // Warmest (CEILING_RGB_WARM)
  }

  Serial.printf("[BLE] adjustCeilingTemp: step=%d -> RGB(%d,%d,%d)\n", ceilingTempStep, r, g, b);
  return setCeilingColor(r, g, b);
}

void BleManager::setCeilingMac(const String& mac) {
  ceilingMac = mac;
  ceilingMac.trim();
  saveSettingsToNVS();
  Serial.printf("[BLE] Ceiling Lights MAC updated to: %s\n", ceilingMac.c_str());
  if (pCeilingClient && pCeilingClient->isConnected()) {
    pCeilingClient->disconnect();
  }
  ensureCeilingConnected();
}

String BleManager::scanNearbyDevices(uint32_t durationSec) {
  Serial.printf("[BLE] Starting BLE scan (%u s)...\n", durationSec);
  NimBLEScan* pScan = NimBLEDevice::getScan();
  pScan->setActiveScan(true);
  pScan->setInterval(100);
  pScan->setWindow(99);

  NimBLEScanResults results = pScan->start(durationSec, false);
  String json = "[";
  int count = results.getCount();
  bool first = true;
  for (int i = 0; i < count; i++) {
    NimBLEAdvertisedDevice dev = results.getDevice(i);
    String name = dev.getName().c_str();
    String addr = dev.getAddress().toString().c_str();
    int rssi = dev.getRSSI();
    String sUuid = dev.haveServiceUUID() ? dev.getServiceUUID().toString().c_str() : "";

    if (!first) json += ",";
    first = false;
    json += "{\"name\":\"" + name + "\",\"mac\":\"" + addr + "\",\"rssi\":" + String(rssi) + ",\"service\":\"" + sUuid + "\"}";
  }
  json += "]";
  pScan->clearResults();
  Serial.printf("[BLE] Scan complete. Found %d devices.\n", count);
  return json;
}

// -----------------------------------------------------------------------------
// Active State Lock Enforcement
// -----------------------------------------------------------------------------
void BleManager::enforceFloorState() {
  if (userReleased || !isFloorConnected() || isEnforcing) return;
  isEnforcing = true;
  lastCommandTime = millis();
  Serial.printf("[BLE State Lock] Enforcing Floor Lamp state: ON=%d\n", floorTargetOn);

  if (floorTargetOn) {
    if (floorMode == MODE_WHITE_CCT) {
      uint8_t cctPacket[] = { 0x7E, 0x06, 0x05, 0x02, floorWarm, floorCool, 0xFF, 0x08, 0xEF };
      sendRaw(pFloorClient, pFloorChar, cctPacket, sizeof(cctPacket));
    } else {
      uint8_t rgbPacket[] = { 0x7E, 0x07, 0x05, 0x03, floorR, floorG, floorB, 0x10, 0xEF };
      sendRaw(pFloorClient, pFloorChar, rgbPacket, sizeof(rgbPacket));
    }
  } else {
    const uint8_t p1[] = { 0x7E, 0x04, 0x04, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xEF };
    sendRaw(pFloorClient, pFloorChar, p1, sizeof(p1));
    delay(40);
    const uint8_t p2[] = { 0x7E, 0x00, 0x04, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xEF };
    sendRaw(pFloorClient, pFloorChar, p2, sizeof(p2));
  }
  isEnforcing = false;
}

void BleManager::handleBleNotification(NimBLERemoteCharacteristic* pChar, uint8_t* data, size_t len) {
  if (userReleased || !pChar || isEnforcing) return;

  unsigned long now = millis();
  // If an ESP32 command was sent within the last 800ms, ignore self-echoes
  if (now - lastCommandTime < 800) {
    return;
  }

  Serial.printf("[BLE State Lock] Floor Lamp notification received on Char %s (%d bytes): ",
    pChar->getUUID().toString().c_str(), (int)len);
  for (size_t i = 0; i < len; i++) {
    if (data[i] < 0x10) Serial.print('0');
    Serial.print(data[i], HEX);
    Serial.print(' ');
  }
  Serial.println();

  enforceFloorState();
}

// -----------------------------------------------------------------------------
// Hybrid Management
// -----------------------------------------------------------------------------
void BleManager::releaseBle() {
  Serial.println(F("[BLE] User requested BLE release. Disconnecting clients..."));
  userReleased = true;

  if (pFloorClient && pFloorClient->isConnected()) {
    pFloorClient->disconnect();
  }
  pFloorChar = nullptr;
  pFloorNotifyChar = nullptr;

  if (pDonutClient && pDonutClient->isConnected()) {
    pDonutClient->disconnect();
  }
  pDonutChar = nullptr;

  if (pCeilingClient && pCeilingClient->isConnected()) {
    pCeilingClient->disconnect();
  }
  pCeilingChar = nullptr;
}

void BleManager::reconnectBle() {
  Serial.println(F("[BLE] Reconnecting BLE devices..."));
  userReleased = false;
  ensureFloorConnected();
  ensureDonutConnected();
  ensureCeilingConnected();
}

bool BleManager::isFloorConnected() {
  return pFloorClient && pFloorClient->isConnected() && pFloorChar != nullptr;
}

bool BleManager::isDonutConnected() {
  return pDonutClient && pDonutClient->isConnected() && pDonutChar != nullptr;
}

bool BleManager::isCeilingConnected() {
  return pCeilingClient && pCeilingClient->isConnected() && pCeilingChar != nullptr;
}

#endif // ESP32
