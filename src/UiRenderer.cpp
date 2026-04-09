#include "UiRenderer.h"

void UiRenderer::begin(bool flip) {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(DISPLAY_I2C_CLOCK_HZ);
  _display.setI2CAddress(DISPLAY_I2C_ADDRESS << 1);
  _display.begin();
  if (flip) {
    _display.setFlipMode(1);
  }
  _display.clearBuffer();
  _display.sendBuffer();
}

void UiRenderer::renderNormal(const BoseState& state,
                              bool wifiConnected,
                              const String& overlayText,
                              const String& statusHint) {
  _display.clearBuffer();

  _display.setFont(u8g2_font_6x12_tf);
  _display.drawStr(0, 10, wifiConnected ? "WiFi:OK" : "WiFi:DOWN");
  _display.drawStr(64, 10, state.wsConnected ? "WS:OK" : "WS:POLL");

  if (!overlayText.isEmpty()) {
    _display.setDrawColor(1);
    _display.drawRFrame(2, 14, 124, 20, 4);
    _display.setFont(u8g2_font_7x14B_tf);
    const String overlay = fitToWidth(overlayText, 112);
    _display.drawUTF8(8, 29, overlay.c_str());
  }

  if (!wifiConnected || !state.connected) {
    _display.setFont(u8g2_font_7x14B_tf);
    _display.drawUTF8(0, 44, "Reconnecting...");
    _display.setFont(u8g2_font_6x12_tf);
    const String hint = fitToWidth(statusHint, 126);
    _display.drawUTF8(0, 58, hint.c_str());
    _display.sendBuffer();
    return;
  }

  _display.setFont(u8g2_font_7x14B_tf);
  const String sourceLabel = fitToWidth(
      state.currentSourceLabel.isEmpty() ? state.currentSourceId : state.currentSourceLabel, 126);
  _display.drawUTF8(0, 44, sourceLabel.c_str());

  _display.setFont(u8g2_font_6x12_tf);
  String volumeLine = "Vol ";
  volumeLine += String(state.volume);
  volumeLine += state.muted ? " MUTE" : "";
  _display.drawUTF8(0, 56, volumeLine.c_str());

  String nowLine = state.artist;
  if (!state.artist.isEmpty() && !state.track.isEmpty()) {
    nowLine += " - ";
  }
  nowLine += state.track;
  nowLine = fitToWidth(nowLine, 126);
  _display.drawUTF8(0, 64, nowLine.c_str());

  _display.sendBuffer();
}

void UiRenderer::renderMenu(const BoseState& state,
                            bool wifiConnected,
                            const UiMenuModel& menu,
                            const String& statusHint) {
  _display.clearBuffer();

  _display.setFont(u8g2_font_6x12_tf);
  _display.drawStr(0, 10, wifiConnected ? "WiFi:OK" : "WiFi:DOWN");
  _display.drawStr(64, 10, state.wsConnected ? "WS:OK" : "WS:POLL");

  _display.setFont(u8g2_font_7x14B_tf);
  _display.drawUTF8(0, 24, fitToWidth(menu.title, 126).c_str());

  const int totalItems = static_cast<int>(menu.items.size());
  const int visibleItems = 3;
  int selectedIndex = menu.selectedIndex;
  if (selectedIndex < 0) {
    selectedIndex = 0;
  }
  if (selectedIndex >= totalItems && totalItems > 0) {
    selectedIndex = totalItems - 1;
  }

  int startIndex = 0;
  if (selectedIndex >= visibleItems) {
    startIndex = selectedIndex - visibleItems + 1;
  }
  if (startIndex + visibleItems > totalItems) {
    startIndex = max(0, totalItems - visibleItems);
  }

  _display.setFont(u8g2_font_6x12_tf);
  for (int row = 0; row < visibleItems && startIndex + row < totalItems; ++row) {
    const int itemIndex = startIndex + row;
    const int y = 38 + row * 10;
    const bool selected = itemIndex == selectedIndex;
    String line = menu.items[itemIndex].label;
    if (!menu.items[itemIndex].enabled) {
      line += " *";
    }

    if (selected) {
      _display.setDrawColor(1);
      _display.drawBox(0, y - 9, 128, 10);
      _display.setDrawColor(0);
      _display.drawUTF8(2, y, fitToWidth(line, 124).c_str());
      _display.setDrawColor(1);
    } else {
      _display.drawUTF8(2, y, fitToWidth(line, 124).c_str());
    }
  }

  String footer = menu.detail;
  if (footer.isEmpty()) {
    footer = statusHint;
  }
  _display.setFont(u8g2_font_5x8_tf);
  _display.drawUTF8(0, 64, fitToWidth(footer, 126).c_str());
  _display.sendBuffer();
}

void UiRenderer::renderSetup(const String& apName, const String& apIp, const String& message) {
  _display.clearBuffer();
  _display.setFont(u8g2_font_7x14B_tf);
  _display.drawUTF8(0, 14, "Setup Mode");

  _display.setFont(u8g2_font_6x12_tf);
  _display.drawUTF8(0, 30, fitToWidth(apName, 126).c_str());
  _display.drawUTF8(0, 42, fitToWidth(apIp, 126).c_str());
  _display.drawUTF8(0, 54, fitToWidth(message, 126).c_str());
  _display.drawUTF8(0, 64, "Open 192.168.4.1");
  _display.sendBuffer();
}

String UiRenderer::fitToWidth(const String& text, uint16_t width) {
  String trimmed = text;
  trimmed.trim();
  if (trimmed.isEmpty()) {
    return trimmed;
  }

  bool truncated = false;
  while (_display.getUTF8Width(trimmed.c_str()) > width && trimmed.length() > 4) {
    trimmed.remove(trimmed.length() - 1);
    truncated = true;
  }

  if (truncated && trimmed.length() > 3) {
    trimmed.remove(trimmed.length() - 3);
    trimmed += "...";
  }

  return trimmed;
}
