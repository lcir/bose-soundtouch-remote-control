#include <Arduino.h>
#include <WiFi.h>

#include "BoseClient.h"
#include "CaptivePortal.h"
#include "ConfigStore.h"
#include "ControlWebServer.h"
#include "InputController.h"
#include "PinConfig.h"
#include "UiRenderer.h"

namespace {
constexpr unsigned long kUiRefreshMs = 80;
constexpr unsigned long kWifiRetryMs = 10000;
constexpr unsigned long kBoseRetryMs = 10000;
constexpr unsigned long kVolumeCommitDelayMs = 100;
constexpr unsigned long kSourceOverlayMs = 2000;
constexpr unsigned long kActionOverlayMs = 500;
}  // namespace

class BoseRemoteApp {
 public:
  void setup() {
    Serial.begin(115200);
    delay(200);

    _inputs.begin();
    initPowerLed();
    _ui.begin(false);

    const bool forceSetup = _inputs.powerHeldDuringBoot(BOOT_SERVICE_HOLD_MS);
    if (!forceSetup && _configStore.load(_config)) {
      _ui.begin(_config.displayFlip);
      beginStationMode();
      return;
    }

    startSetupMode(forceSetup ? "Service button held" : "Missing config");
  }

  void loop() {
    _inputs.update();

    if (_setupMode) {
      handleSetupLoop();
      return;
    }

    handleConnectivity();
    _bose.loop();
    _controlWeb.loop();
    handleInputs();
    flushPendingVolume();
    updatePowerLed();
    renderNormalUi();
  }

 private:
  void initPowerLed() {
    pinMode(PIN_LED_POWER_RED, OUTPUT);
    pinMode(PIN_LED_POWER_GREEN, OUTPUT);
    writePowerLed(false, false);
  }

  void writePowerLed(bool redOn, bool greenOn) {
    const uint8_t redLevel = POWER_LED_ACTIVE_HIGH ? (redOn ? HIGH : LOW) : (redOn ? LOW : HIGH);
    const uint8_t greenLevel =
        POWER_LED_ACTIVE_HIGH ? (greenOn ? HIGH : LOW) : (greenOn ? LOW : HIGH);
    digitalWrite(PIN_LED_POWER_RED, redLevel);
    digitalWrite(PIN_LED_POWER_GREEN, greenLevel);
  }

  void updatePowerLed() {
    const BoseState& state = _bose.state();
    const bool powerOn = state.connected && state.poweredOn;
    writePowerLed(!powerOn, powerOn);
  }

  void startSetupMode(const String& reason) {
    _setupMode = true;
    _bose.disconnect();
    _controlWeb.stop();
    WiFi.disconnect(true, true);
    writePowerLed(true, false);

    const uint64_t chipId = ESP.getEfuseMac();
    const String apName = "BoseRemote-" + String(static_cast<uint16_t>(chipId & 0xFFFF), HEX);
    _portal.start(apName);
    showOverlay(reason, kSourceOverlayMs);
  }

  void beginStationMode() {
    _setupMode = false;
    _portal.stop();
    _controlWeb.stop();
    _lastWifiConnected = false;
    _lastWiFiAttemptMs = 0;
    _lastBoseAttemptMs = 0;
    attemptWiFi();
  }

  void attemptWiFi() {
    if (!_config.isValid()) {
      return;
    }

    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    WiFi.begin(_config.wifiSsid.c_str(), _config.wifiPassword.c_str());
    _lastWiFiAttemptMs = millis();
  }

  void handleSetupLoop() {
    _portal.loop();

    DeviceConfig newConfig;
    if (_portal.consumeSavedConfig(newConfig)) {
      _config = newConfig;
      _configStore.save(_config);
      _ui.begin(_config.displayFlip);
      beginStationMode();
      return;
    }

    renderSetupUi();
  }

  void handleConnectivity() {
    const bool wifiConnected = WiFi.status() == WL_CONNECTED;

    if (wifiConnected && !_lastWifiConnected) {
      _lastWifiConnected = true;
      _controlWeb.start(_config);
      _lastBoseAttemptMs = millis();
      _bose.connect(_config);
    } else if (!wifiConnected && _lastWifiConnected) {
      _lastWifiConnected = false;
      _controlWeb.stop();
      _bose.disconnect();
    }

    if (!wifiConnected && millis() - _lastWiFiAttemptMs >= kWifiRetryMs) {
      attemptWiFi();
    }

    if (wifiConnected && !_bose.state().connected && millis() - _lastBoseAttemptMs >= kBoseRetryMs) {
      _lastBoseAttemptMs = millis();
      _bose.connect(_config);
    }
  }

  void handleInputs() {
    if (_inputs.consumeSourcePressed()) {
      String label;
      if (_bose.selectRelativeSource(1, &label)) {
        showOverlay(label, kSourceOverlayMs);
      }
    }

    if (_inputs.consumePowerPressed()) {
      if (_bose.state().poweredOn && _bose.standby()) {
        showOverlay("Standby", kActionOverlayMs);
      }
    }

    const int delta = _inputs.readEncoderDelta();
    if (delta != 0) {
      const int base = _targetVolume >= 0 ? _targetVolume : _bose.state().volume;
      _targetVolume = constrain(base + delta, 0, 100);
      _lastVolumeChangeMs = millis();
      showOverlay("Vol " + String(_targetVolume), kActionOverlayMs);
    }
  }

  void flushPendingVolume() {
    if (_targetVolume < 0) {
      return;
    }

    if (millis() - _lastVolumeChangeMs < kVolumeCommitDelayMs) {
      return;
    }

    const bool applyMuteState = _bose.state().muted && _targetVolume > 0;
    _bose.setVolume(static_cast<uint8_t>(_targetVolume), applyMuteState, false);
    _targetVolume = -1;
  }

  void renderSetupUi() {
    static unsigned long lastRenderMs = 0;
    if (millis() - lastRenderMs < kUiRefreshMs) {
      return;
    }
    lastRenderMs = millis();

    _ui.renderSetup(_portal.apName(), WiFi.softAPIP().toString(), "Join AP and save config");
  }

  void renderNormalUi() {
    static unsigned long lastRenderMs = 0;
    if (millis() - lastRenderMs < kUiRefreshMs) {
      return;
    }
    lastRenderMs = millis();

    BoseState view = _bose.state();
    if (_targetVolume >= 0) {
      view.volume = static_cast<uint8_t>(_targetVolume);
      if (_targetVolume > 0) {
        view.muted = false;
      }
    }

    const bool wifiConnected = WiFi.status() == WL_CONNECTED;
    String statusHint;
    if (!wifiConnected) {
      statusHint = "Waiting for WiFi";
    } else if (!view.connected) {
      statusHint = "Waiting for Bose host";
    } else if (!view.wsConnected) {
      statusHint = "Polling fallback active";
    }

    const String overlay = millis() < _overlayUntilMs ? _overlayText : "";
    _ui.renderNormal(view, wifiConnected, overlay, statusHint);
  }

  void showOverlay(const String& text, unsigned long durationMs) {
    _overlayText = text;
    _overlayUntilMs = millis() + durationMs;
  }

  ConfigStore _configStore;
  DeviceConfig _config;
  InputController _inputs;
  UiRenderer _ui;
  BoseClient _bose;
  ControlWebServer _controlWeb{_bose};
  CaptivePortal _portal;

  bool _setupMode = false;
  bool _lastWifiConnected = false;
  unsigned long _lastWiFiAttemptMs = 0;
  unsigned long _lastBoseAttemptMs = 0;
  int _targetVolume = -1;
  unsigned long _lastVolumeChangeMs = 0;
  String _overlayText;
  unsigned long _overlayUntilMs = 0;
};

BoseRemoteApp app;

void setup() {
  app.setup();
}

void loop() {
  app.loop();
}
