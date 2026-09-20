#pragma once

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include "hardware_config.h"

// Command execution step types for non-blocking state machine
enum StepType {
  STEP_IDLE,
  STEP_IR_NEC,
  STEP_IR_BEDSIDE,
  STEP_IR_RAW,
  STEP_RF_HIGH,
  STEP_RF_LOW,
  STEP_DELAY
};

struct QueueStep {
  StepType type;
  uint8_t pin;               // RF GPIO pin
  uint16_t necAddress;       // NEC Address
  uint8_t necCommand;        // NEC Command
  uint32_t bedsideData;      // Bedside Lamp pulse payload
  const uint16_t* rawData;   // Raw microsecond pulse array
  uint16_t rawLen;           // Length of raw array
  uint16_t repeat;           // Protocol repeat count
  unsigned long durationMs;  // Time to hold before next step
  const char* label;         // Diagnostic logging label
};

// Current assumed states of all lights
struct DeviceStates {
  bool posterOn;
  bool donutOn;
  bool bedsideOn;
  bool overheadOn;
};

// Queue Engine & Hardware Interface
class CommandQueue {
public:
  CommandQueue();

  void begin();
  void update();

  // Dispatch high-level actions (from Web API or Serial)
  bool dispatchAction(const String& action);

  // Manual state resync (modifies state without transmitting RF/IR)
  void setDeviceState(const String& device, bool state);

  // Direct pulse transmission for testing
  void sendBedsideRawDirect(uint32_t data, uint16_t repeat = 1);
  void sendRawDirect(const uint16_t* buf, uint16_t len, uint16_t repeat = 0);
  void enqueueRFPulse(uint8_t pin, const char* label);
  void triggerRFAsync(uint8_t pin, const char* label = nullptr);

  // State change notification
  typedef void (*StateChangeCallback)(const DeviceStates& states);
  void setStateChangeCallback(StateChangeCallback cb) { stateChangeCb = cb; }
  void notifyStateChanged();

  // State inspection
  const DeviceStates& getStates() const { return states; }
  String getStatesJson() const;
  bool isBusy() const;

private:
  static const size_t QUEUE_CAPACITY = 48;
  QueueStep queue[QUEUE_CAPACITY];
  size_t queueHead;
  size_t queueTail;
  size_t queueCount;

  unsigned long lastStepTime;
  unsigned long stepDuration;

  // Background non-blocking RF pulse tracking
  uint8_t rfActivePin;
  unsigned long rfPulseEndTime;

  StateChangeCallback stateChangeCb;

  DeviceStates states;

  bool enqueue(const QueueStep& step);
  bool isQueueEmpty() const { return queueCount == 0; }
  bool isQueueFull() const { return queueCount >= QUEUE_CAPACITY; }
  QueueStep dequeue();

  void executeStep(const QueueStep& step);

  // Hardware drivers
  void sendBedsideRaw(uint32_t data, uint16_t repeat = 1);
  void enqueueMacroAllOn();
  void enqueueMacroAllOff();
};

extern CommandQueue cmdQueue;
