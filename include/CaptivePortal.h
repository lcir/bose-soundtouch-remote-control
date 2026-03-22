#pragma once

#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>

#include "Types.h"

class CaptivePortal {
 public:
  CaptivePortal();

  void start(const String& apName);
  void stop();
  void loop();

  bool isActive() const;
  const String& apName() const;
  bool consumeSavedConfig(DeviceConfig& config);

 private:
  void registerRoutes();
  void handleRoot();
  void handleSave();
  void redirectToPortal();

  DNSServer _dnsServer;
  WebServer _server;
  bool _active = false;
  bool _hasPendingConfig = false;
  String _apName;
  DeviceConfig _pendingConfig;
};

