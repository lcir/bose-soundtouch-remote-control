#pragma once

#include <U8g2lib.h>
#include <Wire.h>

#include "PinConfig.h"
#include "Types.h"

class UiRenderer {
 public:
  void begin(bool flip);
  void renderNormal(const BoseState& state,
                    bool wifiConnected,
                    const String& overlayText,
                    const String& statusHint);
  void renderMenu(const BoseState& state,
                  bool wifiConnected,
                  const UiMenuModel& menu,
                  const String& statusHint);
  void renderSetup(const String& apName, const String& apIp, const String& message);
  void renderUpdate(const String& title, int percent, const String& detail);
  void renderUpdatePrompt(const String& latestVersion, bool installSelected);
  void renderBusy(const String& title, const String& detail);

 private:
  String fitToWidth(const String& text, uint16_t width);

  U8G2_SSD1306_128X64_NONAME_F_HW_I2C _display =
      U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE);
};
