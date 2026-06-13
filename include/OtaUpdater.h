#pragma once

#include <Arduino.h>

#include <functional>

// Pull-based firmware self-update from GitHub Releases.
//
// The device asks the GitHub API for the latest published release, compares the
// release tag against the firmware version baked in at build time, and (on
// request) downloads the release's firmware.bin asset and flashes it via the
// ESP32 OTA mechanism. All Bose traffic stays local; this is the only feature
// that talks to the public internet, and only when explicitly triggered.
struct OtaStatus {
  String currentVersion;  // Version compiled into this image (FIRMWARE_VERSION).
  String latestVersion;   // tag_name of the newest GitHub release (once checked).
  String downloadUrl;     // browser_download_url of the firmware.bin asset.
  bool updateAvailable = false;
  bool checked = false;   // True once a check has completed (success or failure).
  bool checking = false;  // True while a check is in flight.
  bool applying = false;  // True while a download/flash is in progress.
  String lastError;       // Human readable error from the last check/apply.
};

class OtaUpdater {
 public:
  // progress callback receives a 0..100 percentage during a flash.
  using ProgressCallback = std::function<void(int)>;

  // assetName is the release asset to flash (e.g. "firmware-lolin_s2_mini.bin"),
  // so a multi-board release can ship one .bin per board and each device grabs
  // its own. Falls back to the first .bin asset if no exact match is found.
  void begin(const String& currentVersion, const String& owner, const String& repo,
             const String& assetName);

  // Queries the GitHub API for the latest release. Blocking (a few seconds over
  // TLS). Returns true if the query succeeded; sets updateAvailable when the
  // remote tag is newer than the running version.
  bool checkForUpdate();

  // Downloads the latest known firmware.bin and flashes it. On success the chip
  // reboots into the new image and this call never returns. Returns false on
  // failure (status().lastError describes why). Requires a prior successful
  // checkForUpdate() that found an asset URL.
  bool applyUpdate(ProgressCallback progress = nullptr);

  const OtaStatus& status() const { return _status; }

 private:
  bool fetchLatestRelease(String& tag, String& binUrl, String& err);

  OtaStatus _status;
  String _owner;
  String _repo;
  String _assetName;
};
