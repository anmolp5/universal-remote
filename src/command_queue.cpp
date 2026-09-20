#include "command_queue.h"

// Global IR sender instance on designated GPIO pin
static IRsend irsend(IR_SEND_PIN);

// Global CommandQueue instance
CommandQueue cmdQueue;

CommandQueue::CommandQueue()
  : queueHead(0), queueTail(0), queueCount(0),
    lastStepTime(0), stepDuration(0),
    rfActivePin(0), rfPulseEndTime(0),
    stateChangeCb(nullptr) {
  states.posterOn   = false;
  states.donutOn    = false;
  states.bedsideOn  = false;
  states.floorOn    = false;
  states.overheadOn = false;
}

void CommandQueue::begin() {
  irsend.begin();

  const uint8_t rfPins[] = {
    RF_PIN_POWER,
    RF_PIN_WARMER,
    RF_PIN_DIMMER,
    RF_PIN_COOLER,
    RF_PIN_BRIGHTER
  };

  for (size_t i = 0; i < sizeof(rfPins); i++) {
    digitalWrite(rfPins[i], LOW);
    pinMode(rfPins[i], OUTPUT);
    digitalWrite(rfPins[i], LOW);
  }

  Serial.println(F("[Hardware] IR and RF GPIOs configured."));
}

bool CommandQueue::enqueue(const QueueStep& step) {
  if (isQueueFull()) {
    Serial.println(F("[Queue Warning] Command queue is full, dropping step!"));
    return false;
  }

  queue[queueHead] = step;
  queueHead = (queueHead + 1) % QUEUE_CAPACITY;
  queueCount++;
  return true;
}

QueueStep CommandQueue::dequeue() {
  QueueStep step = { STEP_IDLE, 0, 0, 0, 0, nullptr, 0, 0, 0, nullptr };
  if (isQueueEmpty()) return step;

  step = queue[queueTail];
  queueTail = (queueTail + 1) % QUEUE_CAPACITY;
  queueCount--;
  return step;
}

bool CommandQueue::isBusy() const {
  return queueCount > 0 || (millis() - lastStepTime < stepDuration) || (rfActivePin != 0);
}

void CommandQueue::sendBedsideRaw(uint32_t data, uint16_t repeat) {
  irsend.sendGeneric(
    1300, 400,   // Header Mark, Header Space
    400, 1250,   // One Mark, One Space
    1250, 400,   // Zero Mark, Zero Space
    400, 8000,   // Footer Mark, Gap
    data,        // Data payload
    10,          // 10 bits
    38,          // 38 kHz
    false,       // LSB first
    repeat,      // Repeat count (0 = 1 frame, 1 = 2 frames, 2 = 3 frames)
    50           // Duty cycle percentage (50%)
  );
}

void CommandQueue::executeStep(const QueueStep& step) {
  if (step.label) {
    Serial.print(F("[Action] "));
    Serial.println(step.label);
  }

  switch (step.type) {
    case STEP_IR_NEC: {
      uint32_t necCode = irsend.encodeNEC(step.necAddress, step.necCommand);
      irsend.sendNEC(necCode, 32, 0);
      for (uint16_t r = 0; r < step.repeat; r++) {
        delay(40);
        irsend.sendNEC(necCode, 32, 0);
      }
      break;
    }

    case STEP_IR_BEDSIDE:
      sendBedsideRaw(step.bedsideData, step.repeat);
      break;

    case STEP_IR_RAW:
      if (step.rawData && step.rawLen > 0) {
        for (uint16_t r = 0; r <= step.repeat; r++) {
          irsend.sendRaw(step.rawData, step.rawLen, 38);
          if (r < step.repeat) {
            delay(10);
          }
        }
      }
      break;

    case STEP_RF_HIGH:
      digitalWrite(step.pin, HIGH);
      break;

    case STEP_RF_LOW:
      digitalWrite(step.pin, LOW);
      break;

    case STEP_DELAY:
    case STEP_IDLE:
    default:
      break;
  }
}

void CommandQueue::update() {
  unsigned long now = millis();

  // Background non-blocking release of active RF pin
  if (rfActivePin != 0 && now >= rfPulseEndTime) {
    digitalWrite(rfActivePin, LOW);
    rfActivePin = 0;
  }

  // If currently waiting for an RF hold or cooldown delay, yield execution
  if (now - lastStepTime < stepDuration) {
    return;
  }

  if (isQueueEmpty()) {
    stepDuration = 0;
    return;
  }

  QueueStep step = dequeue();
  executeStep(step);
  lastStepTime = millis();
  stepDuration = step.durationMs;
}

void CommandQueue::triggerRFAsync(uint8_t pin, const char* label) {
  if (label) {
    Serial.print(F("[RF Async Action] "));
    Serial.println(label);
  }
  // If an RF pulse was already active on another pin, bring it LOW first
  if (rfActivePin != 0 && rfActivePin != pin) {
    digitalWrite(rfActivePin, LOW);
  }
  digitalWrite(pin, HIGH);
  rfActivePin = pin;
  rfPulseEndTime = millis() + RF_PRESS_DURATION_MS;
}

void CommandQueue::enqueueRFPulse(uint8_t pin, const char* label) {
  QueueStep highStep = { STEP_RF_HIGH, pin, 0, 0, 0, nullptr, 0, 0, RF_PRESS_DURATION_MS, label };
  enqueue(highStep);

  QueueStep lowStep  = { STEP_RF_LOW,  pin, 0, 0, 0, nullptr, 0, 0, RF_COOLDOWN_MS, nullptr };
  enqueue(lowStep);
}

void CommandQueue::enqueueMacroAllOn() {
  Serial.println(F("[Macro] Queuing Smart ALL ON (Instant Concurrent)..."));

  // 1. RF Floor Lamp - Trigger IMMEDIATELY at t=0 concurrently with IR
  if (!states.floorOn && !states.overheadOn) {
    triggerRFAsync(RF_PIN_POWER, "Floor Lamp (Smart Turn ON - Concurrent)");
    states.floorOn = true;
    states.overheadOn = true;
  } else {
    Serial.println(F("[Macro] Floor Lamp already assumed ON, skipping toggle."));
  }

  // 2. Donut Lamp ON (Trigger FIRST in IR queue: NEC 0x0000 / 0x40)
  QueueStep donutStep  = { STEP_IR_NEC, 0, DONUT_ADDR,  DONUT_CMD_ON,  0, nullptr, 0, 0, MACRO_STEP_DELAY_MS, "Donut Lamp ON" };
  enqueue(donutStep);
  states.donutOn = true;

  // 3. Poster Light ON (NEC 0x7386 / 0x03)
  QueueStep posterStep = { STEP_IR_NEC, 0, POSTER_ADDR, POSTER_CMD_ON, 0, nullptr, 0, 0, MACRO_STEP_DELAY_MS, "Poster Light ON" };
  enqueue(posterStep);
  states.posterOn = true;

  // 4. Bedside Lamp (Pulse distance protocol: only toggle if currently OFF)
  if (!states.bedsideOn) {
    QueueStep bedsideStep = { STEP_IR_BEDSIDE, 0, 0, 0, BEDSIDE_CMD_POWER, nullptr, 0, 1, MACRO_STEP_DELAY_MS, "Bedside Lamp (Smart Turn ON)" };
    enqueue(bedsideStep);
    states.bedsideOn = true;
  } else {
    Serial.println(F("[Macro] Bedside Lamp already assumed ON, skipping toggle."));
  }
}

void CommandQueue::enqueueMacroAllOff() {
  Serial.println(F("[Macro] Queuing Smart ALL OFF (Instant Concurrent)..."));

  // 1. RF Floor Lamp - Trigger IMMEDIATELY at t=0 concurrently with IR
  if (states.floorOn || states.overheadOn) {
    triggerRFAsync(RF_PIN_POWER, "Floor Lamp (Smart Turn OFF - Concurrent)");
    states.floorOn = false;
    states.overheadOn = false;
  } else {
    Serial.println(F("[Macro] Floor Lamp already assumed OFF, skipping toggle."));
  }

  // 2. Donut Lamp OFF (Trigger FIRST in IR queue: NEC 0x0000 / 0x41)
  QueueStep donutStep  = { STEP_IR_NEC, 0, DONUT_ADDR,  DONUT_CMD_OFF,  0, nullptr, 0, 0, MACRO_STEP_DELAY_MS, "Donut Lamp OFF" };
  enqueue(donutStep);
  states.donutOn = false;

  // 3. Poster Light OFF (NEC 0x7386 / 0x98)
  QueueStep posterStep = { STEP_IR_NEC, 0, POSTER_ADDR, POSTER_CMD_OFF, 0, nullptr, 0, 0, MACRO_STEP_DELAY_MS, "Poster Light OFF" };
  enqueue(posterStep);
  states.posterOn = false;

  // 4. Bedside Lamp (Only toggle if currently ON)
  if (states.bedsideOn) {
    QueueStep bedsideStep = { STEP_IR_BEDSIDE, 0, 0, 0, BEDSIDE_CMD_POWER, nullptr, 0, 1, MACRO_STEP_DELAY_MS, "Bedside Lamp (Smart Turn OFF)" };
    enqueue(bedsideStep);
    states.bedsideOn = false;
  } else {
    Serial.println(F("[Macro] Bedside Lamp already assumed OFF, skipping toggle."));
  }
}

void CommandQueue::sendBedsideRawDirect(uint32_t data, uint16_t repeat) {
  sendBedsideRaw(data, repeat);
}

void CommandQueue::sendRawDirect(const uint16_t* buf, uint16_t len, uint16_t repeat) {
  if (!buf || len == 0) return;
  for (uint16_t r = 0; r <= repeat; r++) {
    irsend.sendRaw(buf, len, 38);
    if (r < repeat) {
      delay(10);
    }
  }
}

void CommandQueue::notifyStateChanged() {
  if (stateChangeCb) {
    stateChangeCb(states);
  }
}

void CommandQueue::setDeviceState(const String& device, bool state) {
  if (device.equalsIgnoreCase("bedside")) {
    states.bedsideOn = state;
    Serial.printf("[Resync] Bedside Lamp state manually set to %s\n", state ? "ON" : "OFF");
  } else if (device.equalsIgnoreCase("floor") || device.equalsIgnoreCase("overhead")) {
    states.floorOn = state;
    states.overheadOn = state;
    Serial.printf("[Resync] Floor Lamp state manually set to %s\n", state ? "ON" : "OFF");
  } else if (device.equalsIgnoreCase("poster")) {
    states.posterOn = state;
    Serial.printf("[Resync] Poster Light state manually set to %s\n", state ? "ON" : "OFF");
  } else if (device.equalsIgnoreCase("donut")) {
    states.donutOn = state;
    Serial.printf("[Resync] Donut Lamp state manually set to %s\n", state ? "ON" : "OFF");
  }
  notifyStateChanged();
}

String CommandQueue::getStatesJson() const {
  char buf[180];
  snprintf(buf, sizeof(buf),
    "{\"poster\":%s,\"donut\":%s,\"bedside\":%s,\"floor\":%s,\"overhead\":%s,\"busy\":%s}",
    states.posterOn ? "true" : "false",
    states.donutOn ? "true" : "false",
    states.bedsideOn ? "true" : "false",
    states.floorOn ? "true" : "false",
    states.overheadOn ? "true" : "false",
    isBusy() ? "true" : "false"
  );
  return String(buf);
}

bool CommandQueue::dispatchAction(const String& action) {
  bool handled = false;

  // Poster Light
  if (action.equalsIgnoreCase("poster_on") || action == "1") {
    QueueStep s = { STEP_IR_NEC, 0, POSTER_ADDR, POSTER_CMD_ON, 0, nullptr, 0, 0, 20, "Poster Light ON" };
    enqueue(s);
    states.posterOn = true;
    handled = true;
  } else if (action.equalsIgnoreCase("poster_off") || action == "2") {
    QueueStep s = { STEP_IR_NEC, 0, POSTER_ADDR, POSTER_CMD_OFF, 0, nullptr, 0, 0, 20, "Poster Light OFF" };
    enqueue(s);
    states.posterOn = false;
    handled = true;
  } else if (action.equalsIgnoreCase("poster_color") || action == "3") {
    QueueStep s = { STEP_IR_NEC, 0, POSTER_ADDR, POSTER_CMD_COLOR, 0, nullptr, 0, 0, 20, "Poster Light COLOR" };
    enqueue(s);
    handled = true;
  }

  // Donut Lamp
  else if (action.equalsIgnoreCase("donut_on") || action == "4") {
    QueueStep s = { STEP_IR_NEC, 0, DONUT_ADDR, DONUT_CMD_ON, 0, nullptr, 0, 0, 20, "Donut Lamp ON" };
    enqueue(s);
    states.donutOn = true;
    handled = true;
  } else if (action.equalsIgnoreCase("donut_off") || action == "5") {
    QueueStep s = { STEP_IR_NEC, 0, DONUT_ADDR, DONUT_CMD_OFF, 0, nullptr, 0, 0, 20, "Donut Lamp OFF" };
    enqueue(s);
    states.donutOn = false;
    handled = true;
  }

  // Bedside Lamp: Power, Brighter, Dimmer, Warmer, Cooler
  else if (action.equalsIgnoreCase("bedside_power") || action == "6") {
    QueueStep s = { STEP_IR_BEDSIDE, 0, 0, 0, BEDSIDE_CMD_POWER, nullptr, 0, 1, 30, "Bedside Lamp Power Toggle" };
    enqueue(s);
    states.bedsideOn = !states.bedsideOn;
    handled = true;
  } else if (action.equalsIgnoreCase("bedside_brighter") || action == "7") {
    QueueStep s = { STEP_IR_BEDSIDE, 0, 0, 0, BEDSIDE_CMD_BRIGHTER, nullptr, 0, 1, 30, "Bedside Lamp Brighter (0x3B2)" };
    enqueue(s);
    handled = true;
  } else if (action.equalsIgnoreCase("bedside_dimmer") || action == "8") {
    QueueStep s = { STEP_IR_RAW, 0, 0, 0, 0, RAW_BEDSIDE_DIMMER, RAW_BEDSIDE_DIMMER_LEN, 0, 30, "Bedside Lamp Dimmer (191-pulse capture)" };
    enqueue(s);
    handled = true;
  } else if (action.equalsIgnoreCase("bedside_warmer") || action == "9") {
    QueueStep s = { STEP_IR_BEDSIDE, 0, 0, 0, BEDSIDE_CMD_WARMER, nullptr, 0, 1, 30, "Bedside Lamp Warmer" };
    enqueue(s);
    handled = true;
  } else if (action.equalsIgnoreCase("bedside_cooler") || action == "0") {
    QueueStep s = { STEP_IR_BEDSIDE, 0, 0, 0, BEDSIDE_CMD_COOLER, nullptr, 0, 1, 30, "Bedside Lamp Cooler" };
    enqueue(s);
    handled = true;
  }

  // RF Floor Lamp (formerly Overhead)
  else if (action.equalsIgnoreCase("floor_power") || action.equalsIgnoreCase("rf_power") || action.equalsIgnoreCase("overhead_power") || action == "q") {
    enqueueRFPulse(RF_PIN_POWER, "RF Floor Lamp Power Toggle");
    states.floorOn = !states.floorOn;
    states.overheadOn = states.floorOn;
    handled = true;
  } else if (action.equalsIgnoreCase("floor_warmer") || action.equalsIgnoreCase("rf_warmer") || action.equalsIgnoreCase("overhead_warmer") || action == "w") {
    enqueueRFPulse(RF_PIN_WARMER, "RF Floor Lamp Warmer");
    handled = true;
  } else if (action.equalsIgnoreCase("floor_dimmer") || action.equalsIgnoreCase("rf_dimmer") || action.equalsIgnoreCase("overhead_dimmer") || action == "e") {
    enqueueRFPulse(RF_PIN_DIMMER, "RF Floor Lamp Dimmer");
    handled = true;
  } else if (action.equalsIgnoreCase("floor_cooler") || action.equalsIgnoreCase("rf_cooler") || action.equalsIgnoreCase("overhead_cooler") || action == "r") {
    enqueueRFPulse(RF_PIN_COOLER, "RF Floor Lamp Cooler");
    handled = true;
  } else if (action.equalsIgnoreCase("floor_brighter") || action.equalsIgnoreCase("rf_brighter") || action.equalsIgnoreCase("overhead_brighter") || action == "t") {
    enqueueRFPulse(RF_PIN_BRIGHTER, "RF Floor Lamp Brighter");
    handled = true;
  }

  // Master Macros
  else if (action.equalsIgnoreCase("all_on") || action == "a") {
    enqueueMacroAllOn();
    handled = true;
  } else if (action.equalsIgnoreCase("all_off") || action == "o") {
    enqueueMacroAllOff();
    handled = true;
  }

  // State Resynchronization Actions (Local HTTP & Cloud MQTT)
  else if (action.equalsIgnoreCase("sync_bedside_on")) {
    setDeviceState("bedside", true);
    return true;
  } else if (action.equalsIgnoreCase("sync_bedside_off")) {
    setDeviceState("bedside", false);
    return true;
  } else if (action.equalsIgnoreCase("sync_floor_on") || action.equalsIgnoreCase("sync_overhead_on")) {
    setDeviceState("floor", true);
    return true;
  } else if (action.equalsIgnoreCase("sync_floor_off") || action.equalsIgnoreCase("sync_overhead_off")) {
    setDeviceState("floor", false);
    return true;
  } else if (action.equalsIgnoreCase("sync_poster_on")) {
    setDeviceState("poster", true);
    return true;
  } else if (action.equalsIgnoreCase("sync_poster_off")) {
    setDeviceState("poster", false);
    return true;
  } else if (action.equalsIgnoreCase("sync_donut_on")) {
    setDeviceState("donut", true);
    return true;
  } else if (action.equalsIgnoreCase("sync_donut_off")) {
    setDeviceState("donut", false);
    return true;
  }

  if (handled) {
    notifyStateChanged();
    return true;
  }

  Serial.print(F("[Warning] Unknown action: "));
  Serial.println(action);
  return false;
}
