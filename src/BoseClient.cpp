#include "BoseClient.h"

namespace {
constexpr const char* kKeySender = "Gabbo";
constexpr const char* kSelectionOnline = "ONLINE";
constexpr const char* kSelectionBluetooth = "BLUETOOTH";

String xmlDecode(String value) {
  value.replace("&amp;", "&");
  value.replace("&lt;", "<");
  value.replace("&gt;", ">");
  value.replace("&quot;", "\"");
  value.replace("&apos;", "'");
  value.trim();
  return value;
}

String extractTagValue(const String& xml, const char* tagName) {
  const String openTag = "<" + String(tagName);
  const int openStart = xml.indexOf(openTag);
  if (openStart < 0) {
    return "";
  }

  const int contentStart = xml.indexOf('>', openStart);
  if (contentStart < 0) {
    return "";
  }

  const String closeTag = "</" + String(tagName) + ">";
  const int closeStart = xml.indexOf(closeTag, contentStart + 1);
  if (closeStart < 0) {
    return "";
  }

  return xmlDecode(xml.substring(contentStart + 1, closeStart));
}

String extractAttributeValue(const String& element, const char* attribute) {
  const String token = String(attribute) + "=\"";
  const int start = element.indexOf(token);
  if (start < 0) {
    return "";
  }

  const int valueStart = start + token.length();
  const int valueEnd = element.indexOf('"', valueStart);
  if (valueEnd < 0) {
    return "";
  }

  return xmlDecode(element.substring(valueStart, valueEnd));
}

String makePath(const String& path) {
  if (path.startsWith("/")) {
    return path;
  }
  return "/" + path;
}
}  // namespace

BoseClient::BoseClient() {
  _ws.onEvent([this](WStype_t type, uint8_t* payload, size_t length) {
    handleWebSocketEvent(type, payload, length);
  });
  _ws.setReconnectInterval(5000);
}

bool BoseClient::connect(const DeviceConfig& config) {
  if (!config.isValid()) {
    return false;
  }

  _config = config;
  _configured = true;
  _state.connected = false;
  _state.wsConnected = false;
  _lastActiveSelectionId = "";
  _lastPollMs = millis();
  _pendingRefreshMask = RefreshAllMask;
  beginWebSocket();
  return refreshAll();
}

void BoseClient::disconnect() {
  if (_wsStarted) {
    _ws.disconnect();
  }

  _wsStarted = false;
  _configured = false;
  _state.connected = false;
  _state.wsConnected = false;
  _state.poweredOn = false;
  _state.track = "";
  _state.artist = "";
  _state.currentSourceId = "";
  _state.currentSourceLabel = "";
  _lastActiveSelectionId = "";
  _sources.clear();
}

void BoseClient::loop() {
  if (!_configured) {
    return;
  }

  if (_wsStarted) {
    _ws.loop();
  }

  processScheduledRefreshes();

  if (!_state.wsConnected && millis() - _lastPollMs >= 5000) {
    refreshAll();
    _lastPollMs = millis();
  }
}

bool BoseClient::refreshAll() {
  if (!_configured) {
    return false;
  }

  bool ok = refreshInfo();
  ok = refreshSources() || ok;
  ok = refreshVolume() || ok;
  ok = refreshNowPlaying() || ok;
  _state.connected = ok;
  return ok;
}

bool BoseClient::setVolume(uint8_t value, bool applyMuteState, bool muted) {
  String body = "<volume>";
  body += String(value);
  if (applyMuteState) {
    body += "<muteenabled>";
    body += muted ? "true" : "false";
    body += "</muteenabled>";
  }
  body += "</volume>";

  if (!httpPost("/volume", body)) {
    return false;
  }

  scheduleRefresh(RefreshVolume);
  return true;
}

bool BoseClient::selectSource(const String& sourceId) {
  const int sourceIndex = findSourceIndexForSelectionId(sourceId);
  if (sourceIndex < 0) {
    return false;
  }

  const BoseSource& source = _sources[sourceIndex];
  if (!source.ready) {
    if (source.transportSource.equalsIgnoreCase(kSelectionBluetooth)) {
      return enterBluetoothPairing();
    }
    if (!source.selectable) {
      return false;
    }
  }

  String body = "<ContentItem source=\"";
  body += source.transportSource;
  body += "\"";
  if (!source.sourceAccount.isEmpty()) {
    body += " sourceAccount=\"";
    body += source.sourceAccount;
    body += "\"";
  }
  body += "></ContentItem>";

  if (!httpPost("/select", body)) {
    return false;
  }

  scheduleRefresh(RefreshSources | RefreshNowPlaying);
  return true;
}

bool BoseClient::selectRelativeSource(int direction, String* selectedLabel) {
  if (_sources.empty()) {
    return false;
  }

  std::vector<String> selectionIds;
  std::vector<String> selectionLabels;
  for (const auto& source : _sources) {
    if (!source.selectable) {
      continue;
    }

    const String selectionId = selectionIdForSource(source);
    if (selectionId.isEmpty()) {
      continue;
    }

    bool exists = false;
    for (const auto& existingId : selectionIds) {
      if (existingId == selectionId) {
        exists = true;
        break;
      }
    }
    if (exists) {
      continue;
    }

    selectionIds.push_back(selectionId);
    selectionLabels.push_back(selectionLabelForSource(source));
  }

  const int count = static_cast<int>(selectionIds.size());
  if (count == 0) {
    return false;
  }

  const String currentId = currentSelectionId();
  int currentIndex = -1;
  for (int i = 0; i < count; ++i) {
    if (selectionIds[i] == currentId) {
      currentIndex = i;
      break;
    }
  }

  if (currentIndex < 0) {
    const int start = direction >= 0 ? 0 : count - 1;
    const int step = direction >= 0 ? 1 : -1;
    for (int candidate = start; candidate >= 0 && candidate < count; candidate += step) {
      if (selectedLabel != nullptr) {
        *selectedLabel = selectionLabels[candidate];
      }
      return selectSource(selectionIds[candidate]);
    }
    return false;
  }

  for (int offset = 1; offset <= count; ++offset) {
    const int candidate = (currentIndex + direction * offset + count) % count;
    if (selectedLabel != nullptr) {
      *selectedLabel = selectionLabels[candidate];
    }
    return selectSource(selectionIds[candidate]);
  }

  return false;
}

bool BoseClient::sendKey(const String& keyName) {
  String press = "<key state=\"press\" sender=\"";
  press += kKeySender;
  press += "\">";
  press += keyName;
  press += "</key>";

  String release = "<key state=\"release\" sender=\"";
  release += kKeySender;
  release += "\">";
  release += keyName;
  release += "</key>";

  if (!httpPost("/key", press)) {
    return false;
  }
  if (!httpPost("/key", release)) {
    return false;
  }

  scheduleRefresh(RefreshAllMask);
  return true;
}

bool BoseClient::wake() {
  bool changed = sendKey("POWER");
  delay(150);
  refreshAll();
  if (_state.poweredOn) {
    return true;
  }

  const int wakeIndex = findPreferredWakeSourceIndex();
  if (wakeIndex >= 0) {
    changed = selectSource(selectionIdForSource(_sources[wakeIndex])) || changed;
    delay(150);
    refreshAll();
  }

  return changed && _state.connected;
}

bool BoseClient::standby() {
  if (!httpGetOk("/standby")) {
    return false;
  }

  _state.poweredOn = false;
  scheduleRefresh(RefreshAllMask);
  return true;
}

bool BoseClient::enterBluetoothPairing() {
  if (!httpGetOk("/enterBluetoothPairing")) {
    return false;
  }

  scheduleRefresh(RefreshSources | RefreshNowPlaying);
  return true;
}

String BoseClient::currentSelectionId() const {
  if (!_state.poweredOn || _state.currentSourceId.isEmpty()) {
    return "";
  }

  for (const auto& source : _sources) {
    if (source.id == _state.currentSourceId) {
      return selectionIdForSource(source);
    }
  }

  if (_state.currentSourceId.equalsIgnoreCase(kSelectionBluetooth)) {
    return kSelectionBluetooth;
  }

  if (_state.currentSourceId.startsWith("AUX")) {
    return _state.currentSourceId;
  }

  return kSelectionOnline;
}

String BoseClient::selectionIdForSource(const BoseSource& source) const {
  if (!source.visible) {
    return "";
  }

  if (source.transportSource.equalsIgnoreCase(kSelectionBluetooth)) {
    return kSelectionBluetooth;
  }

  if (isOnlineSource(source)) {
    return kSelectionOnline;
  }

  return source.id;
}

String BoseClient::selectionLabelForSource(const BoseSource& source) const {
  const String selectionId = selectionIdForSource(source);
  if (selectionId.equalsIgnoreCase(kSelectionBluetooth)) {
    return "Bluetooth";
  }

  if (selectionId.equalsIgnoreCase(kSelectionOnline)) {
    return "Online";
  }

  return makeSourceLabel(source);
}

const BoseState& BoseClient::state() const {
  return _state;
}

const std::vector<BoseSource>& BoseClient::sources() const {
  return _sources;
}

String BoseClient::httpGet(const String& path) {
  WiFiClient wifiClient;
  HTTPClient http;
  const String url = "http://" + _config.boseHost + ":" + String(_config.boseHttpPort) + makePath(path);
  if (!http.begin(wifiClient, url)) {
    return "";
  }

  const int status = http.GET();
  if (status <= 0 || status >= 400) {
    http.end();
    return "";
  }

  const String body = http.getString();
  http.end();
  return body;
}

bool BoseClient::httpGetOk(const String& path) {
  WiFiClient wifiClient;
  HTTPClient http;
  const String url = "http://" + _config.boseHost + ":" + String(_config.boseHttpPort) + makePath(path);
  if (!http.begin(wifiClient, url)) {
    return false;
  }

  const int status = http.GET();
  http.end();
  return status > 0 && status < 400;
}

bool BoseClient::httpPost(const String& path, const String& body) {
  WiFiClient wifiClient;
  HTTPClient http;
  const String url = "http://" + _config.boseHost + ":" + String(_config.boseHttpPort) + makePath(path);
  if (!http.begin(wifiClient, url)) {
    return false;
  }

  http.addHeader("Content-Type", "text/xml; charset=utf-8");
  const int status = http.POST(body);
  http.end();
  return status > 0 && status < 400;
}

void BoseClient::beginWebSocket() {
  if (_wsStarted) {
    _ws.disconnect();
    _wsStarted = false;
  }

  _ws.begin(_config.boseHost.c_str(), _config.boseWsPort, "/", "gabbo");
  _wsStarted = true;
}

void BoseClient::handleWebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      _state.wsConnected = true;
      scheduleRefresh(RefreshAllMask);
      break;
    case WStype_DISCONNECTED:
      _state.wsConnected = false;
      break;
    case WStype_TEXT: {
      String text;
      text.reserve(length);
      for (size_t i = 0; i < length; ++i) {
        text += static_cast<char>(payload[i]);
      }
      if (text.indexOf("sourcesUpdated") >= 0) {
        scheduleRefresh(RefreshSources);
      }
      if (text.indexOf("volumeUpdated") >= 0) {
        scheduleRefresh(RefreshVolume);
      }
      if (text.indexOf("nowPlayingUpdated") >= 0) {
        scheduleRefresh(RefreshNowPlaying);
      }
      if (text.indexOf("zoneUpdated") >= 0) {
        scheduleRefresh(RefreshAllMask);
      }
      break;
    }
    default:
      break;
  }
}

void BoseClient::scheduleRefresh(uint8_t refreshMask) {
  _pendingRefreshMask |= refreshMask;
  _lastRefreshRequestMs = millis();
}

void BoseClient::processScheduledRefreshes() {
  if (_pendingRefreshMask == RefreshNone) {
    return;
  }

  if (millis() - _lastRefreshRequestMs < 50) {
    return;
  }

  const uint8_t mask = _pendingRefreshMask;
  _pendingRefreshMask = RefreshNone;

  bool ok = _state.connected;
  if (mask & RefreshSources) {
    ok = refreshSources() || ok;
  }
  if (mask & RefreshVolume) {
    ok = refreshVolume() || ok;
  }
  if (mask & RefreshNowPlaying) {
    ok = refreshNowPlaying() || ok;
  }

  _state.connected = ok;
}

int BoseClient::findSourceIndexForSelectionId(const String& selectionId, bool preferCurrent) const {
  if (selectionId.isEmpty()) {
    return -1;
  }

  int firstSelectableIndex = -1;
  for (size_t i = 0; i < _sources.size(); ++i) {
    const BoseSource& source = _sources[i];
    if (!source.visible || selectionIdForSource(source) != selectionId) {
      continue;
    }

    if (preferCurrent && source.id == _state.currentSourceId) {
      return static_cast<int>(i);
    }

    if (source.ready) {
      return static_cast<int>(i);
    }

    if (firstSelectableIndex < 0 && source.selectable) {
      firstSelectableIndex = static_cast<int>(i);
    }
  }

  return firstSelectableIndex;
}

int BoseClient::findPreferredWakeSourceIndex() const {
  if (!_lastActiveSelectionId.isEmpty()) {
    const int previousIndex = findSourceIndexForSelectionId(_lastActiveSelectionId, false);
    if (previousIndex >= 0) {
      return previousIndex;
    }
  }

  const String preferredIds[] = {"AUX", "AUX2", "AUX3", kSelectionOnline, kSelectionBluetooth};
  for (const String& preferredId : preferredIds) {
    const int preferredIndex = findSourceIndexForSelectionId(preferredId, false);
    if (preferredIndex >= 0) {
      return preferredIndex;
    }
  }

  for (size_t i = 0; i < _sources.size(); ++i) {
    if (_sources[i].selectable) {
      return static_cast<int>(i);
    }
  }

  return -1;
}

bool BoseClient::isVisibleSource(const BoseSource& source) const {
  if (source.transportSource.equalsIgnoreCase("STANDBY") ||
      source.transportSource.equalsIgnoreCase("NOTIFICATION")) {
    return false;
  }

  if (isHiddenSource(source)) {
    return false;
  }

  return !source.id.isEmpty() || !source.transportSource.isEmpty();
}

bool BoseClient::isSelectableSource(const BoseSource& source) const {
  if (!isVisibleSource(source)) {
    return false;
  }

  if (source.ready) {
    return true;
  }

  if (source.transportSource.equalsIgnoreCase("BLUETOOTH")) {
    return true;
  }

  if (isOnlineSource(source)) {
    return true;
  }

  return false;
}

bool BoseClient::isOnlineSource(const BoseSource& source) const {
  return !source.isLocal && !source.transportSource.equalsIgnoreCase(kSelectionBluetooth) &&
         !source.transportSource.equalsIgnoreCase("STANDBY") &&
         !source.transportSource.equalsIgnoreCase("NOTIFICATION");
}

bool BoseClient::isHiddenSource(const BoseSource& source) const {
  String id = source.id;
  String label = source.label;
  String transport = source.transportSource;
  id.toUpperCase();
  label.toUpperCase();
  transport.toUpperCase();

  return id.startsWith("QPLAY") || label.startsWith("QPLAY") || transport.startsWith("QPLAY");
}

String BoseClient::makeSourceLabel(const BoseSource& source) {
  if (!source.label.isEmpty()) {
    return source.label;
  }

  String label = source.transportSource;
  if (label.isEmpty()) {
    label = source.id;
  }

  label.replace("_", " ");
  if (!label.isEmpty()) {
    label.toLowerCase();
    label[0] = static_cast<char>(toupper(label[0]));
  }
  return label;
}

bool BoseClient::refreshInfo() {
  const String xml = httpGet("/info");
  return !xml.isEmpty();
}

bool BoseClient::refreshSources() {
  const String xml = httpGet("/sources");
  if (xml.isEmpty()) {
    return false;
  }

  std::vector<BoseSource> parsed;
  int cursor = 0;
  while (true) {
    const int tagStart = xml.indexOf("<sourceItem", cursor);
    if (tagStart < 0) {
      break;
    }

    const int tagEnd = xml.indexOf('>', tagStart);
    if (tagEnd < 0) {
      break;
    }

    const String element = xml.substring(tagStart, tagEnd + 1);
    const bool selfClosing = tagEnd > tagStart && xml[tagEnd - 1] == '/';
    const int closeTag = selfClosing ? tagEnd : xml.indexOf("</sourceItem>", tagEnd);
    if (!selfClosing && closeTag < 0) {
      break;
    }

    BoseSource source;
    source.transportSource = extractAttributeValue(element, "source");
    source.sourceAccount = extractAttributeValue(element, "sourceAccount");
    source.status = extractAttributeValue(element, "status");
    source.ready = source.status.equalsIgnoreCase("READY");
    source.isLocal = extractAttributeValue(element, "isLocal").equalsIgnoreCase("true");
    source.id = source.sourceAccount.isEmpty() ? source.transportSource : source.sourceAccount;
    if (selfClosing) {
      source.label = makeSourceLabel(source);
    } else {
      source.label = xmlDecode(xml.substring(tagEnd + 1, closeTag));
      source.label = makeSourceLabel(source);
    }
    source.visible = isVisibleSource(source);
    source.selectable = isSelectableSource(source);
    parsed.push_back(source);
    cursor = selfClosing ? tagEnd + 1 : closeTag + 13;
  }

  _sources = parsed;
  return !_sources.empty();
}

bool BoseClient::refreshVolume() {
  const String xml = httpGet("/volume");
  if (xml.isEmpty()) {
    return false;
  }

  const String actual = extractTagValue(xml, "actualvolume");
  const String muted = extractTagValue(xml, "muteenabled");
  _state.volume = static_cast<uint8_t>(actual.toInt());
  _state.muted = muted.equalsIgnoreCase("true");
  return true;
}

bool BoseClient::refreshNowPlaying() {
  const String xml = httpGet("/now_playing");
  if (xml.isEmpty()) {
    return false;
  }

  const int nowPlayingStart = xml.indexOf("<nowPlaying");
  if (nowPlayingStart >= 0) {
    const int nowPlayingEnd = xml.indexOf('>', nowPlayingStart);
    if (nowPlayingEnd > nowPlayingStart) {
      const String root = xml.substring(nowPlayingStart, nowPlayingEnd + 1);
      const String source = extractAttributeValue(root, "source");

      const int contentStart = xml.indexOf("<ContentItem");
      const int contentEnd = xml.indexOf('>', contentStart);
      String sourceAccount;
      if (contentStart >= 0 && contentEnd > contentStart) {
        const String contentItem = xml.substring(contentStart, contentEnd + 1);
        sourceAccount = extractAttributeValue(contentItem, "sourceAccount");
      }

      _state.currentSourceId = sourceAccount.isEmpty() ? source : sourceAccount;
      _state.poweredOn = !_state.currentSourceId.equalsIgnoreCase("STANDBY") && !_state.currentSourceId.isEmpty();
    }
  }

  _state.track = extractTagValue(xml, "track");
  _state.artist = extractTagValue(xml, "artist");

  if (!_state.poweredOn) {
    _state.currentSourceLabel = "Standby";
    _state.track = "";
    _state.artist = "";
    return true;
  }

  _state.currentSourceLabel = _state.currentSourceId;
  bool matchedSource = false;
  for (const auto& source : _sources) {
    if (source.id == _state.currentSourceId) {
      _state.currentSourceLabel = selectionLabelForSource(source);
      _lastActiveSelectionId = selectionIdForSource(source);
      matchedSource = true;
      break;
    }
  }

  if (!matchedSource) {
    if (_state.currentSourceId.equalsIgnoreCase(kSelectionBluetooth)) {
      _state.currentSourceLabel = "Bluetooth";
      _lastActiveSelectionId = kSelectionBluetooth;
    } else if (!_state.currentSourceId.startsWith("AUX")) {
      _state.currentSourceLabel = "Online";
      _lastActiveSelectionId = kSelectionOnline;
    } else {
      _lastActiveSelectionId = _state.currentSourceId;
    }
  }

  return true;
}
