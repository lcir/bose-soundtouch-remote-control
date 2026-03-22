#pragma once

#include <WebServer.h>

#include "BoseClient.h"
#include "Types.h"

class ControlWebServer {
 public:
  explicit ControlWebServer(BoseClient& boseClient);

  void start(const DeviceConfig& config);
  void stop();
  void loop();

  bool isActive() const;

 private:
  void registerRoutes();
  void handleRoot();
  void handleState();
  void handleSourceNext();
  void handleSourceSelect();
  void handlePowerAction();
  void handleStandby();
  void handleVolume();
  void sendJsonState();
  void sendJsonResult(bool ok, const String& message = "");

  String buildPage() const;
  String buildStateJson() const;
  static String jsonEscape(const String& value);

  BoseClient& _boseClient;
  WebServer _server;
  DeviceConfig _config;
  bool _active = false;
};
