#pragma once

#include <stdint.h>

// Default prototype wiring for LOLIN/Wemos S2 Mini.
// The OLED is a generic 0.96" 128x64 I2C SSD1306-compatible module.
constexpr int PIN_I2C_SDA = 33;
constexpr int PIN_I2C_SCL = 35;
constexpr uint8_t DISPLAY_I2C_ADDRESS = 0x3C;
constexpr uint32_t DISPLAY_I2C_CLOCK_HZ = 400000;

constexpr int PIN_ENCODER_A = 7;
constexpr int PIN_ENCODER_B = 9;
constexpr int PIN_ENCODER_BUTTON = 11;

constexpr int PIN_BUTTON_SOURCE = 5;
constexpr int PIN_LED_STATUS = 18;
constexpr bool STATUS_LED_ACTIVE_HIGH = true;

constexpr unsigned long BOOT_SERVICE_HOLD_MS = 1500;
constexpr unsigned long BUTTON_DEBOUNCE_MS = 25;
