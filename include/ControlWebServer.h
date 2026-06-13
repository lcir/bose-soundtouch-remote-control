#pragma once

#include <WebServer.h>

#include "BoseClient.h"
#include "OtaUpdater.h"
#include "Types.h"

class ControlWebServer {
 public:
  ControlWebServer(BoseClient& boseClient, OtaUpdater& ota);

  void start(const DeviceConfig& config);
  void stop();
  void loop();

  bool isActive() const;

  // True once between an /api/ota/apply request and the main loop picking it up.
  // The main loop owns the (blocking, reboot-on-success) flash so the HTTP
  // response can be sent first.
  bool consumeOtaApplyRequest();

 private:
  void registerRoutes();
  void handleRoot();
  void handleState();
  void handleSourceNext();
  void handleSourceSelect();
  void handlePowerAction();
  void handleStandby();
  void handleVolume();
  void handleOtaStatus();
  void handleOtaCheck();
  void handleOtaApply();
  void sendJsonState();
  void sendJsonResult(bool ok, const String& message = "");

  String buildPage() const;
  String buildStateJson() const;
  String buildOtaJson() const;
  static String jsonEscape(const String& value);

  BoseClient& _boseClient;
  OtaUpdater& _ota;
  WebServer _server;
  DeviceConfig _config;
  bool _active = false;
  bool _otaApplyRequested = false;
};
