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
