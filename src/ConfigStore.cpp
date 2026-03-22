#include "ConfigStore.h"

bool ConfigStore::load(DeviceConfig& config) {
  Preferences preferences;
  if (!preferences.begin(kNamespace, true)) {
    return false;
  }

  config.wifiSsid = preferences.getString("wifi_ssid", "");
  config.wifiPassword = preferences.getString("wifi_pwd", "");
  config.boseHost = preferences.getString("bose_host", "");
  config.boseHttpPort = preferences.getUShort("http_port", 8090);
  config.boseWsPort = preferences.getUShort("ws_port", 8080);
  config.displayFlip = preferences.getBool("display_flip", false);
  preferences.end();

  return config.isValid();
}

bool ConfigStore::save(const DeviceConfig& config) {
  Preferences preferences;
  if (!preferences.begin(kNamespace, false)) {
    return false;
  }

  preferences.putString("wifi_ssid", config.wifiSsid);
  preferences.putString("wifi_pwd", config.wifiPassword);
  preferences.putString("bose_host", config.boseHost);
  preferences.putUShort("http_port", config.boseHttpPort);
  preferences.putUShort("ws_port", config.boseWsPort);
  preferences.putBool("display_flip", config.displayFlip);
  preferences.end();

  return true;
}

void ConfigStore::clear() {
  Preferences preferences;
  if (preferences.begin(kNamespace, false)) {
    preferences.clear();
    preferences.end();
  }
}

