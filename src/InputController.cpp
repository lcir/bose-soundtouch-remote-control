#include <stdlib.h>

#include "InputController.h"

namespace {
constexpr int kEncoderCountsPerStep = 2;
}

void InputController::begin() {
  initButton(_encoderButton, PIN_ENCODER_BUTTON);

  ESP32Encoder::useInternalWeakPullResistors = puType::up;
  _encoder.attachHalfQuad(PIN_ENCODER_A, PIN_ENCODER_B);
  _encoder.setCount(0);
  _lastEncoderCount = 0;
}

void InputController::update() {
  updateButton(_encoderButton);
}

bool InputController::powerHeldDuringBoot(unsigned long durationMs) const {
  const unsigned long started = millis();
  while (millis() - started < durationMs) {
    if (digitalRead(PIN_ENCODER_BUTTON) != LOW) {
      return false;
    }
    delay(10);
  }
  return true;
}

bool InputController::consumeEncoderPressed() {
  return consumeButton(_encoderButton);
}

int InputController::readEncoderDelta() {
  const int64_t current = _encoder.getCount();
  const int delta = static_cast<int>(current - _lastEncoderCount);
  _lastEncoderCount = current;

  _pendingEncoderCounts += delta;
  if (abs(_pendingEncoderCounts) < kEncoderCountsPerStep) {
    return 0;
  }

  const int steps = _pendingEncoderCounts / kEncoderCountsPerStep;
  _pendingEncoderCounts %= kEncoderCountsPerStep;
  return steps;
}

void InputController::initButton(ButtonState& button, int pin) {
  button.pin = pin;
  pinMode(pin, INPUT_PULLUP);
  button.stableLevel = digitalRead(pin);
  button.lastReading = button.stableLevel;
  button.lastChangeMs = millis();
}

void InputController::updateButton(ButtonState& button) {
  const bool reading = digitalRead(button.pin);
  if (reading != button.lastReading) {
    button.lastReading = reading;
    button.lastChangeMs = millis();
  }

  if ((millis() - button.lastChangeMs) < BUTTON_DEBOUNCE_MS) {
    return;
  }

  if (reading != button.stableLevel) {
    button.stableLevel = reading;
    if (button.stableLevel == LOW) {
      button.pressedEvent = true;
    }
  }
}

bool InputController::consumeButton(ButtonState& button) {
  if (!button.pressedEvent) {
    return false;
  }

  button.pressedEvent = false;
  return true;
}
