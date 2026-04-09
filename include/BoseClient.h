#pragma once

#include <HTTPClient.h>
#include <WebSocketsClient.h>
#include <WiFiClient.h>

#include <vector>

#include "Types.h"

class BoseClient {
 public:
  BoseClient();

  bool connect(const DeviceConfig& config);
  void disconnect();
  void loop();

  bool refreshAll();
  bool setVolume(uint8_t value, bool applyMuteState = false, bool muted = false);
  bool selectSource(const String& sourceId);
  bool selectRelativeSource(int direction, String* selectedLabel = nullptr);
  bool wake();
  bool standby();
  bool sendKey(const String& keyName);
  bool canSelectSource(const String& sourceId) const;
  String currentSelectionId() const;
  String selectionIdForSource(const BoseSource& source) const;
  String selectionLabelForSource(const BoseSource& source) const;

  const BoseState& state() const;
  const std::vector<BoseSource>& sources() const;

 private:
  enum RefreshMask : uint8_t {
    RefreshNone = 0,
    RefreshSources = 1 << 0,
    RefreshVolume = 1 << 1,
    RefreshNowPlaying = 1 << 2,
    RefreshAllMask = RefreshSources | RefreshVolume | RefreshNowPlaying,
  };

  String httpGet(const String& path);
  bool httpGetOk(const String& path);
  bool httpPost(const String& path, const String& body);
  void beginWebSocket();
  void handleWebSocketEvent(WStype_t type, uint8_t* payload, size_t length);
  void scheduleRefresh(uint8_t refreshMask);
  void processScheduledRefreshes();
  int findSourceIndexForSelectionId(const String& selectionId, bool preferCurrent = true) const;
  int findPreferredWakeSourceIndex() const;
  bool enterBluetoothPairing();
  bool isVisibleSource(const BoseSource& source) const;
  bool isSelectableSource(const BoseSource& source) const;
  bool isOnlineSource(const BoseSource& source) const;
  bool isHiddenSource(const BoseSource& source) const;
  static String makeSourceLabel(const BoseSource& source);
  bool refreshInfo();
  bool refreshSources();
  bool refreshVolume();
  bool refreshNowPlaying();

  DeviceConfig _config;
  bool _configured = false;
  bool _wsStarted = false;
  unsigned long _lastPollMs = 0;
  unsigned long _lastRefreshRequestMs = 0;
  uint8_t _pendingRefreshMask = RefreshNone;
  String _lastActiveSelectionId;

  BoseState _state;
  std::vector<BoseSource> _sources;
  WebSocketsClient _ws;
};
