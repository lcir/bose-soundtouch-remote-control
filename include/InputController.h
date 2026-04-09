#pragma once

#include <Arduino.h>
#include <ESP32Encoder.h>

#include "PinConfig.h"

class InputController {
 public:
  void begin();
  void update();

  bool powerHeldDuringBoot(unsigned long durationMs) const;
  bool consumeEncoderPressed();
  int readEncoderDelta();

 private:
  struct ButtonState {
    int pin = -1;
    bool stableLevel = HIGH;
    bool lastReading = HIGH;
    bool pressedEvent = false;
    unsigned long lastChangeMs = 0;
  };

  void initButton(ButtonState& button, int pin);
  void updateButton(ButtonState& button);
  bool consumeButton(ButtonState& button);

  ButtonState _encoderButton;
  ESP32Encoder _encoder;
  int64_t _lastEncoderCount = 0;
  int _pendingEncoderCounts = 0;
};
