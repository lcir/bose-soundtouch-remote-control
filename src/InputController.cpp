#include "InputController.h"

void InputController::begin() {
  initButton(_sourceButton, PIN_BUTTON_SOURCE);
  initButton(_powerButton, PIN_BUTTON_POWER);

  ESP32Encoder::useInternalWeakPullResistors = puType::up;
  _encoder.attachHalfQuad(PIN_ENCODER_A, PIN_ENCODER_B);
  _encoder.setCount(0);
  _lastEncoderCount = 0;
}

void InputController::update() {
  updateButton(_sourceButton);
  updateButton(_powerButton);
}

bool InputController::powerHeldDuringBoot(unsigned long durationMs) const {
  const unsigned long started = millis();
  while (millis() - started < durationMs) {
    if (digitalRead(PIN_BUTTON_POWER) != LOW) {
      return false;
    }
    delay(10);
  }
  return true;
}

bool InputController::consumeSourcePressed() {
  return consumeButton(_sourceButton);
}

bool InputController::consumePowerPressed() {
  return consumeButton(_powerButton);
}

int InputController::readEncoderDelta() {
  const int64_t current = _encoder.getCount();
  const int delta = static_cast<int>(current - _lastEncoderCount);
  _lastEncoderCount = current;
  return delta;
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
