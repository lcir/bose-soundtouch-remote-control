#include "CaptivePortal.h"

namespace {
constexpr byte kDnsPort = 53;

String htmlHeader(const String& title) {
  String html;
  html += "<!doctype html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>" + title + "</title>";
  html += "<style>";
  html += "body{font-family:sans-serif;background:#f5f2ea;color:#222;margin:0;padding:24px;}";
  html += "main{max-width:32rem;margin:0 auto;background:#fff;padding:24px;border-radius:16px;";
  html += "box-shadow:0 12px 36px rgba(0,0,0,.08);}label{display:block;margin:12px 0 6px;}";
  html += "input{width:100%;padding:12px;border:1px solid #bbb;border-radius:10px;}";
  html += "button{margin-top:16px;padding:12px 16px;border:0;border-radius:10px;";
  html += "background:#0f766e;color:#fff;font-weight:600;width:100%;}";
  html += "small{display:block;margin-top:10px;color:#666;}";
  html += "</style></head><body><main>";
  return html;
}

String htmlFooter() {
  return "</main></body></html>";
}
}  // namespace

CaptivePortal::CaptivePortal() : _server(80) {
  registerRoutes();
}

void CaptivePortal::start(const String& apName) {
  _apName = apName;
  _hasPendingConfig = false;

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(_apName.c_str());

  _dnsServer.start(kDnsPort, "*", WiFi.softAPIP());
  _server.begin();
  _active = true;
}

void CaptivePortal::stop() {
  if (!_active) {
    return;
  }

  _dnsServer.stop();
  _server.stop();
  WiFi.softAPdisconnect(true);
  _active = false;
}

void CaptivePortal::loop() {
  if (!_active) {
    return;
  }

  _dnsServer.processNextRequest();
  _server.handleClient();
}

bool CaptivePortal::isActive() const {
  return _active;
}

const String& CaptivePortal::apName() const {
  return _apName;
}

bool CaptivePortal::consumeSavedConfig(DeviceConfig& config) {
  if (!_hasPendingConfig) {
    return false;
  }

  config = _pendingConfig;
  _hasPendingConfig = false;
  return true;
}

void CaptivePortal::registerRoutes() {
  _server.on("/", HTTP_GET, [this]() { handleRoot(); });
  _server.on("/save", HTTP_POST, [this]() { handleSave(); });
  _server.onNotFound([this]() { redirectToPortal(); });
}

void CaptivePortal::handleRoot() {
  String html = htmlHeader("Bose Remote Setup");
  html += "<h1>Bose Remote Setup</h1>";
  html += "<p>Configure Wi-Fi and the target Bose SoundTouch host.</p>";
  html += "<form method='post' action='/save'>";
  html += "<label for='ssid'>Wi-Fi SSID</label><input id='ssid' name='ssid' required>";
  html += "<label for='password'>Wi-Fi Password</label><input id='password' name='password' type='password'>";
  html += "<label for='bose_host'>Bose Host/IP</label><input id='bose_host' name='bose_host' required>";
  html += "<label for='display_flip'>Display Flip</label>";
  html += "<input id='display_flip' name='display_flip' type='checkbox' value='1'>";
  html += "<button type='submit'>Save Configuration</button>";
  html += "</form>";
  html += "<small>After save, the device will switch to station mode automatically.</small>";
  html += htmlFooter();
  _server.send(200, "text/html", html);
}

void CaptivePortal::handleSave() {
  DeviceConfig config;
  config.wifiSsid = _server.arg("ssid");
  config.wifiPassword = _server.arg("password");
  config.boseHost = _server.arg("bose_host");
  config.displayFlip = _server.hasArg("display_flip");

  config.wifiSsid.trim();
  config.boseHost.trim();

  if (!config.isValid()) {
    _server.send(400, "text/plain", "Missing SSID or Bose host.");
    return;
  }

  _pendingConfig = config;
  _hasPendingConfig = true;

  String html = htmlHeader("Saved");
  html += "<h1>Configuration saved</h1>";
  html += "<p>The ESP32 is switching to Wi-Fi station mode.</p>";
  html += htmlFooter();
  _server.send(200, "text/html", html);
}

void CaptivePortal::redirectToPortal() {
  _server.sendHeader("Location", "http://192.168.4.1/", true);
  _server.send(302, "text/plain", "");
}
