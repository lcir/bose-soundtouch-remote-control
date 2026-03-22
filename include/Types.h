#pragma once

#include <Arduino.h>

#include <vector>

struct DeviceConfig {
  String wifiSsid;
  String wifiPassword;
  String boseHost;
  uint16_t boseHttpPort = 8090;
  uint16_t boseWsPort = 8080;
  bool displayFlip = false;

  bool isValid() const {
    return !wifiSsid.isEmpty() && !boseHost.isEmpty();
  }
};

struct BoseSource {
  String id;
  String label;
  bool ready = false;
  bool visible = false;
  bool selectable = false;
  String status;
  bool isLocal = false;
  String transportSource;
  String sourceAccount;
};

struct BoseState {
  bool connected = false;
  bool wsConnected = false;
  bool poweredOn = false;
  uint8_t volume = 0;
  bool muted = false;
  String currentSourceId;
  String currentSourceLabel;
  String track;
  String artist;
};
