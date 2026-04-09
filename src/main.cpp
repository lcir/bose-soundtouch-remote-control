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
constexpr unsigned long kMenuTimeoutMs = 4000;

constexpr const char* kSelectionOnline = "ONLINE";
constexpr const char* kSelectionBluetooth = "BLUETOOTH";
constexpr const char* kSelectionAux1 = "AUX";
constexpr const char* kSelectionAux2 = "AUX2";
constexpr const char* kSelectionAux3 = "AUX3";
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
  enum class MenuMode : uint8_t {
    Idle,
    Main,
    VolumeEdit,
    SourceSelect,
    PowerSelect,
  };

  struct SourceMenuOption {
    const char* selectionId;
    const char* label;
  };

  static constexpr SourceMenuOption kSourceMenuOptions[5] = {
      {kSelectionOnline, "Online"},
      {kSelectionBluetooth, "Bluetooth"},
      {kSelectionAux1, "Aux1"},
      {kSelectionAux2, "Aux2"},
      {kSelectionAux3, "Aux3"},
  };

  static UiMenuItem makeMenuItem(const String& label, bool enabled) {
    UiMenuItem item;
    item.label = label;
    item.enabled = enabled;
    return item;
  }

  void initPowerLed() {
    pinMode(PIN_LED_STATUS, OUTPUT);
    writeStatusLed(false);
  }

  void writeStatusLed(bool on) {
    const uint8_t level = STATUS_LED_ACTIVE_HIGH ? (on ? HIGH : LOW) : (on ? LOW : HIGH);
    digitalWrite(PIN_LED_STATUS, level);
  }

  void updatePowerLed() {
    const BoseState& state = _bose.state();
    const bool powerOn = state.connected && state.poweredOn;
    writeStatusLed(powerOn);
  }

  void startSetupMode(const String& reason) {
    _setupMode = true;
    _menuMode = MenuMode::Idle;
    _bose.disconnect();
    _controlWeb.stop();
    WiFi.disconnect(true, true);
    writeStatusLed(false);

    const uint64_t chipId = ESP.getEfuseMac();
    const String apName = "BoseRemote-" + String(static_cast<uint16_t>(chipId & 0xFFFF), HEX);
    _portal.start(apName);
    showOverlay(reason, kSourceOverlayMs);
  }

  void beginStationMode() {
    _setupMode = false;
    _menuMode = MenuMode::Idle;
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
    if (_menuMode != MenuMode::Idle && millis() - _lastMenuInteractionMs >= kMenuTimeoutMs) {
      closeMenu();
    }

    const int delta = _inputs.readEncoderDelta();
    if (delta != 0) {
      handleEncoderDelta(delta);
    }

    if (_inputs.consumeEncoderPressed()) {
      handleEncoderPress();
    }
  }

  void handleEncoderDelta(int delta) {
    noteMenuInteraction();

    switch (_menuMode) {
      case MenuMode::Idle:
        adjustPendingVolume(delta, true);
        break;
      case MenuMode::Main:
        _mainMenuIndex = wrapIndex(_mainMenuIndex, delta, 3);
        break;
      case MenuMode::VolumeEdit:
        adjustPendingVolume(delta, false);
        break;
      case MenuMode::SourceSelect:
        _sourceMenuIndex = moveEnabledIndex(_sourceMenuIndex, delta, sourceMenuItems());
        break;
      case MenuMode::PowerSelect:
        _powerMenuIndex = moveEnabledIndex(_powerMenuIndex, delta, powerMenuItems());
        break;
    }
  }

  void handleEncoderPress() {
    noteMenuInteraction();

    switch (_menuMode) {
      case MenuMode::Idle:
        openMainMenu();
        break;
      case MenuMode::Main:
        if (_mainMenuIndex == 0) {
          enterVolumeMenu();
        } else if (_mainMenuIndex == 1) {
          enterSourceMenu();
        } else {
          enterPowerMenu();
        }
        break;
      case MenuMode::VolumeEdit:
        commitPendingVolumeNow();
        showOverlay("Vol " + String(displayVolume()), kActionOverlayMs);
        closeMenu();
        break;
      case MenuMode::SourceSelect:
        applySelectedSource();
        break;
      case MenuMode::PowerSelect:
        applySelectedPower();
        break;
    }
  }

  void adjustPendingVolume(int delta, bool showLiveOverlay) {
    const int base = _targetVolume >= 0 ? _targetVolume : _bose.state().volume;
    const int next = constrain(base + delta, 0, 100);
    if (next == base) {
      return;
    }

    _targetVolume = next;
    _lastVolumeChangeMs = millis();
    if (showLiveOverlay) {
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

    commitPendingVolumeNow();
  }

  void commitPendingVolumeNow() {
    if (_targetVolume < 0) {
      return;
    }

    const bool applyMuteState = _bose.state().muted && _targetVolume > 0;
    _bose.setVolume(static_cast<uint8_t>(_targetVolume), applyMuteState, false);
    _targetVolume = -1;
  }

  void openMainMenu() {
    _mainMenuIndex = 0;
    _menuMode = MenuMode::Main;
    noteMenuInteraction();
  }

  void enterVolumeMenu() {
    if (_targetVolume < 0) {
      _targetVolume = _bose.state().volume;
    }
    _menuMode = MenuMode::VolumeEdit;
    noteMenuInteraction();
  }

  void enterSourceMenu() {
    _sourceMenuIndex = currentSourceMenuIndex();
    _sourceMenuIndex = moveEnabledIndex(_sourceMenuIndex, 0, sourceMenuItems());
    _menuMode = MenuMode::SourceSelect;
    noteMenuInteraction();
  }

  void enterPowerMenu() {
    _powerMenuIndex = _bose.state().poweredOn ? 1 : 0;
    _powerMenuIndex = moveEnabledIndex(_powerMenuIndex, 0, powerMenuItems());
    _menuMode = MenuMode::PowerSelect;
    noteMenuInteraction();
  }

  void closeMenu() {
    _menuMode = MenuMode::Idle;
    _mainMenuIndex = 0;
    _sourceMenuIndex = 0;
    _powerMenuIndex = 0;
    _lastMenuInteractionMs = 0;
  }

  void applySelectedSource() {
    const auto items = sourceMenuItems();
    if (_sourceMenuIndex < 0 || _sourceMenuIndex >= static_cast<int>(items.size()) ||
        !items[_sourceMenuIndex].enabled) {
      showOverlay("Source unavailable", kActionOverlayMs);
      closeMenu();
      return;
    }

    const String sourceId = kSourceMenuOptions[_sourceMenuIndex].selectionId;
    const String label = kSourceMenuOptions[_sourceMenuIndex].label;
    if (_bose.selectSource(sourceId)) {
      showOverlay(label, kSourceOverlayMs);
    } else {
      showOverlay("Source failed", kActionOverlayMs);
    }
    closeMenu();
  }

  void applySelectedPower() {
    const auto items = powerMenuItems();
    if (_powerMenuIndex < 0 || _powerMenuIndex >= static_cast<int>(items.size()) ||
        !items[_powerMenuIndex].enabled) {
      showOverlay("Power unchanged", kActionOverlayMs);
      closeMenu();
      return;
    }

    bool ok = false;
    String label;
    if (_powerMenuIndex == 0) {
      ok = _bose.wake();
      label = "Power On";
    } else {
      ok = _bose.standby();
      label = "Standby";
    }

    showOverlay(ok ? label : "Power failed", kActionOverlayMs);
    closeMenu();
  }

  int displayVolume() const {
    return _targetVolume >= 0 ? _targetVolume : _bose.state().volume;
  }

  int currentSourceMenuIndex() const {
    const String currentId = _bose.currentSelectionId();
    for (int i = 0; i < 5; ++i) {
      if (currentId.equalsIgnoreCase(kSourceMenuOptions[i].selectionId)) {
        return i;
      }
    }
    return 0;
  }

  std::vector<UiMenuItem> sourceMenuItems() const {
    std::vector<UiMenuItem> items;
    items.reserve(5);

    for (const auto& option : kSourceMenuOptions) {
      items.push_back(makeMenuItem(option.label, isSourceSelectionAvailable(option.selectionId)));
    }
    return items;
  }

  std::vector<UiMenuItem> powerMenuItems() const {
    std::vector<UiMenuItem> items;
    items.reserve(2);
    items.push_back(makeMenuItem("On", !_bose.state().poweredOn));
    items.push_back(makeMenuItem("Off", _bose.state().poweredOn));
    return items;
  }

  bool isSourceSelectionAvailable(const String& selectionId) const {
    if (!_bose.state().poweredOn) {
      return false;
    }

    const auto& sources = _bose.sources();
    for (const auto& source : sources) {
      if (_bose.selectionIdForSource(source).equalsIgnoreCase(selectionId) && source.selectable) {
        return true;
      }
    }
    return false;
  }

  int wrapIndex(int current, int delta, int count) const {
    if (count <= 0) {
      return 0;
    }

    int index = current;
    const int direction = delta >= 0 ? 1 : -1;
    for (int step = 0; step < abs(delta); ++step) {
      index = (index + direction + count) % count;
    }
    return index;
  }

  int moveEnabledIndex(int current, int delta, const std::vector<UiMenuItem>& items) const {
    if (items.empty()) {
      return 0;
    }

    int firstEnabled = -1;
    for (size_t i = 0; i < items.size(); ++i) {
      if (items[i].enabled) {
        firstEnabled = static_cast<int>(i);
        break;
      }
    }
    if (firstEnabled < 0) {
      return 0;
    }

    if (current < 0 || current >= static_cast<int>(items.size())) {
      current = firstEnabled;
    }

    if (delta == 0) {
      return items[current].enabled ? current : firstEnabled;
    }

    int index = current;
    const int count = static_cast<int>(items.size());
    const int direction = delta > 0 ? 1 : -1;
    for (int step = 0; step < abs(delta); ++step) {
      for (int attempt = 0; attempt < count; ++attempt) {
        index = (index + direction + count) % count;
        if (items[index].enabled) {
          break;
        }
      }
    }
    return index;
  }

  UiMenuModel currentMenuModel(const String& statusHint) const {
    UiMenuModel menu;

    switch (_menuMode) {
      case MenuMode::Main:
        menu.title = "Menu";
        menu.items.clear();
        menu.items.push_back(makeMenuItem("Volume", true));
        menu.items.push_back(makeMenuItem("Source", true));
        menu.items.push_back(makeMenuItem("Power", true));
        menu.selectedIndex = _mainMenuIndex;
        menu.detail = "Press to open";
        break;
      case MenuMode::VolumeEdit:
        menu.title = "Volume";
        menu.items.clear();
        menu.items.push_back(makeMenuItem(String("Value: ") + String(displayVolume()), true));
        menu.selectedIndex = 0;
        menu.detail = "Turn / press confirm";
        break;
      case MenuMode::SourceSelect:
        menu.title = "Source";
        menu.items = sourceMenuItems();
        menu.selectedIndex = _sourceMenuIndex;
        menu.detail = _bose.state().poweredOn ? "Select source" : "Power on first";
        break;
      case MenuMode::PowerSelect:
        menu.title = "Power";
        menu.items = powerMenuItems();
        menu.selectedIndex = _powerMenuIndex;
        menu.detail = "Select power state";
        break;
      case MenuMode::Idle:
        menu.title = "";
        menu.detail = statusHint;
        break;
    }

    return menu;
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

    if (_menuMode != MenuMode::Idle) {
      _ui.renderMenu(view, wifiConnected, currentMenuModel(statusHint), statusHint);
      return;
    }

    const String overlay = millis() < _overlayUntilMs ? _overlayText : "";
    _ui.renderNormal(view, wifiConnected, overlay, statusHint);
  }

  void showOverlay(const String& text, unsigned long durationMs) {
    _overlayText = text;
    _overlayUntilMs = millis() + durationMs;
  }

  void noteMenuInteraction() {
    if (_menuMode != MenuMode::Idle) {
      _lastMenuInteractionMs = millis();
    }
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

  MenuMode _menuMode = MenuMode::Idle;
  int _mainMenuIndex = 0;
  int _sourceMenuIndex = 0;
  int _powerMenuIndex = 0;
  unsigned long _lastMenuInteractionMs = 0;
};

constexpr BoseRemoteApp::SourceMenuOption BoseRemoteApp::kSourceMenuOptions[5];

BoseRemoteApp app;

void setup() {
  app.setup();
}

void loop() {
  app.loop();
}
