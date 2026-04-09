#include "UiRenderer.h"

namespace {
constexpr int kStatusTopY = 10;
constexpr int kMenuTopY = 14;
constexpr int kMenuHeight = 50;
constexpr int kCardWidth = 92;
constexpr int kCardHeight = 34;
constexpr int kCardRadius = 6;
constexpr int kCenterX = 64;
constexpr int kCardCenterY = kMenuTopY + (kMenuHeight / 2);
constexpr int kCardTextY = kCardCenterY + 5;
constexpr int kSidePeekOffset = 74;
constexpr int kNeighborOffset = 96;

int wrapMenuIndex(int index, int count) {
  if (count <= 0) {
    return 0;
  }
  return (index % count + count) % count;
}

void drawCard(U8G2_SSD1306_128X64_NONAME_F_HW_I2C& display,
              int centerX,
              const String& label,
              bool selected,
              bool enabled) {
  const int left = centerX - (kCardWidth / 2);
  const int top = kCardCenterY - (kCardHeight / 2);

  if (selected && enabled) {
    display.setDrawColor(1);
    display.drawRBox(left, top, kCardWidth, kCardHeight, kCardRadius);
    display.setDrawColor(0);
  } else {
    display.setDrawColor(1);
    display.drawRFrame(left, top, kCardWidth, kCardHeight, kCardRadius);
    if (!enabled) {
      display.drawLine(left + 8, top + 8, left + 18, top + 18);
      display.drawLine(left + 18, top + 8, left + 8, top + 18);
    }
  }

  display.setFont(selected ? u8g2_font_10x20_tf : u8g2_font_7x14B_tf);
  const String fitted = selected ? label : label.substring(0, min(static_cast<unsigned int>(label.length()), 4U));
  const uint16_t width = display.getUTF8Width(fitted.c_str());
  display.drawUTF8(centerX - (width / 2), selected ? kCardTextY : kCardCenterY + 4, fitted.c_str());
  display.setDrawColor(1);
}

void drawCarouselCards(U8G2_SSD1306_128X64_NONAME_F_HW_I2C& display, const UiMenuModel& menu) {
  const int totalItems = static_cast<int>(menu.items.size());
  if (totalItems <= 0) {
    return;
  }

  int currentIndex = wrapMenuIndex(menu.selectedIndex, totalItems);
  int previousIndex = wrapMenuIndex(menu.previousSelectedIndex, totalItems);
  int slideOffset = 0;
  if (menu.transitionActive) {
    slideOffset = (menu.transitionDirection * kNeighborOffset * (255 - menu.transitionProgress)) / 255;
  }

  const int currentCenterX = kCenterX + slideOffset;
  const int previousCenterX = kCenterX + slideOffset - (menu.transitionDirection * kNeighborOffset);

  if (menu.transitionActive && previousIndex != currentIndex) {
    drawCard(display,
             previousCenterX,
             menu.items[previousIndex].label,
             false,
             menu.items[previousIndex].enabled);
  }

  const int leftNeighborIndex = wrapMenuIndex(currentIndex - 1, totalItems);
  const int rightNeighborIndex = wrapMenuIndex(currentIndex + 1, totalItems);
  drawCard(display,
           currentCenterX - kSidePeekOffset,
           menu.items[leftNeighborIndex].label,
           false,
           menu.items[leftNeighborIndex].enabled);
  drawCard(display,
           currentCenterX + kSidePeekOffset,
           menu.items[rightNeighborIndex].label,
           false,
           menu.items[rightNeighborIndex].enabled);
  drawCard(display, currentCenterX, menu.items[currentIndex].label, true, menu.items[currentIndex].enabled);
}
}  // namespace

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
  _display.drawStr(0, kStatusTopY, wifiConnected ? "WiFi:OK" : "WiFi:DOWN");
  _display.drawStr(64, kStatusTopY, state.wsConnected ? "WS:OK" : "WS:POLL");
  _display.drawHLine(0, kMenuTopY - 1, 128);

  if (menu.items.empty()) {
    _display.setFont(u8g2_font_6x12_tf);
    _display.drawUTF8(0, 42, fitToWidth(statusHint, 126).c_str());
    _display.sendBuffer();
    return;
  }

  drawCarouselCards(_display, menu);
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
