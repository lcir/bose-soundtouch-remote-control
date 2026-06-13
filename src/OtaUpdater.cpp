#include "OtaUpdater.h"

#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>

namespace {
constexpr const char* kUserAgent = "bose-soundtouch-remote-control-esp32";
constexpr uint32_t kHttpTimeoutMs = 12000;

// Strips a single leading 'v'/'V' and parses the leading major.minor.patch
// triplet of a tag like "v1.4.2" or "1.4.2-3-gabc123". Missing components read
// as 0, so "v2" -> {2,0,0}. Returns false only for an empty/garbage string.
bool parseSemver(const String& tag, long out[3]) {
  out[0] = out[1] = out[2] = 0;
  int i = 0;
  if (i < (int)tag.length() && (tag[i] == 'v' || tag[i] == 'V')) {
    ++i;
  }
  int component = 0;
  bool sawDigit = false;
  long value = 0;
  for (; i < (int)tag.length() && component < 3; ++i) {
    const char c = tag[i];
    if (c >= '0' && c <= '9') {
      value = value * 10 + (c - '0');
      sawDigit = true;
    } else if (c == '.') {
      out[component++] = value;
      value = 0;
    } else {
      break;  // Stop at '-', '+', etc. (pre-release / build metadata).
    }
  }
  if (component < 3) {
    out[component] = value;
  }
  return sawDigit;
}

// A version is "structured" only if it has the vN.N(.N) shape; a bare git hash
// like "699acdd" is not, even though it starts with digits.
bool looksLikeSemver(const String& tag) {
  long tmp[3];
  return parseSemver(tag, tmp) && tag.indexOf('.') >= 0;
}

// Returns true if `latest` represents a strictly newer release than `current`.
bool isNewer(const String& latest, const String& current) {
  long l[3];
  long c[3];
  if (!looksLikeSemver(latest)) {
    return false;  // Remote tag isn't a version we can reason about.
  }
  parseSemver(latest, l);
  if (!looksLikeSemver(current)) {
    return true;  // Dev/untagged build (e.g. bare hash) -> offer the release.
  }
  parseSemver(current, c);
  for (int i = 0; i < 3; ++i) {
    if (l[i] != c[i]) {
      return l[i] > c[i];
    }
  }
  return false;
}

// Extracts the quoted string value following the first occurrence of
// "key" at or after `from`. Sets `searchEnd` past the consumed value so callers
// can continue scanning. Returns "" when not found.
String jsonStringValue(const String& body, const char* key, int from, int* searchEnd) {
  String needle = "\"";
  needle += key;
  needle += "\"";
  int keyPos = body.indexOf(needle, from);
  if (keyPos < 0) {
    if (searchEnd) {
      *searchEnd = body.length();
    }
    return "";
  }
  int colon = body.indexOf(':', keyPos + needle.length());
  if (colon < 0) {
    if (searchEnd) {
      *searchEnd = body.length();
    }
    return "";
  }
  int quoteOpen = body.indexOf('"', colon + 1);
  if (quoteOpen < 0) {
    if (searchEnd) {
      *searchEnd = body.length();
    }
    return "";
  }
  // Find the closing quote, honouring backslash escapes.
  int j = quoteOpen + 1;
  String value;
  while (j < (int)body.length()) {
    const char c = body[j];
    if (c == '\\' && j + 1 < (int)body.length()) {
      value += body[j + 1];
      j += 2;
      continue;
    }
    if (c == '"') {
      break;
    }
    value += c;
    ++j;
  }
  if (searchEnd) {
    *searchEnd = j + 1;
  }
  return value;
}
}  // namespace

void OtaUpdater::begin(const String& currentVersion, const String& owner, const String& repo,
                       const String& assetName) {
  _status.currentVersion = currentVersion;
  _owner = owner;
  _repo = repo;
  _assetName = assetName;
}

bool OtaUpdater::checkForUpdate() {
  if (WiFi.status() != WL_CONNECTED) {
    _status.lastError = "WiFi not connected";
    _status.checked = true;
    return false;
  }

  _status.checking = true;
  _status.lastError = "";

  String tag;
  String binUrl;
  String err;
  const bool ok = fetchLatestRelease(tag, binUrl, err);

  _status.checking = false;
  _status.checked = true;

  if (!ok) {
    _status.lastError = err;
    return false;
  }

  _status.latestVersion = tag;
  _status.downloadUrl = binUrl;
  _status.updateAvailable = !binUrl.isEmpty() && isNewer(tag, _status.currentVersion);
  return true;
}

bool OtaUpdater::fetchLatestRelease(String& tag, String& binUrl, String& err) {
  WiFiClientSecure client;
  client.setInsecure();  // GitHub certs are valid; skip pinning to save flash/RAM.
  client.setTimeout(kHttpTimeoutMs / 1000);

  HTTPClient http;
  String url = "https://api.github.com/repos/" + _owner + "/" + _repo + "/releases/latest";
  if (!http.begin(client, url)) {
    err = "begin() failed";
    return false;
  }
  http.setTimeout(kHttpTimeoutMs);
  http.setUserAgent(kUserAgent);  // GitHub API rejects requests without a UA.
  http.addHeader("Accept", "application/vnd.github+json");
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    err = "HTTP " + String(code);
    http.end();
    return false;
  }

  const String body = http.getString();
  http.end();

  tag = jsonStringValue(body, "tag_name", 0, nullptr);
  if (tag.isEmpty()) {
    err = "no tag_name in response";
    return false;
  }

  // Walk every browser_download_url. Prefer the asset whose name matches this
  // board's _assetName; otherwise remember the first generic ".bin" as fallback.
  String fallback;
  int cursor = 0;
  while (cursor < (int)body.length()) {
    int next = cursor;
    const String candidate = jsonStringValue(body, "browser_download_url", cursor, &next);
    if (candidate.isEmpty()) {
      break;
    }
    if (!_assetName.isEmpty() && candidate.endsWith("/" + _assetName)) {
      binUrl = candidate;
      break;
    }
    if (fallback.isEmpty() && candidate.endsWith(".bin")) {
      fallback = candidate;
    }
    cursor = next;
  }

  if (binUrl.isEmpty()) {
    binUrl = fallback;
  }
  if (binUrl.isEmpty()) {
    err = "release has no .bin asset";
    return false;
  }
  return true;
}

bool OtaUpdater::applyUpdate(ProgressCallback progress) {
  if (_status.downloadUrl.isEmpty()) {
    _status.lastError = "no download URL (check first)";
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    _status.lastError = "WiFi not connected";
    return false;
  }

  _status.applying = true;
  _status.lastError = "";

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(kHttpTimeoutMs / 1000);

  if (progress) {
    httpUpdate.onProgress([progress](int current, int total) {
      progress(total > 0 ? (int)((current * 100L) / total) : 0);
    });
  }
  httpUpdate.rebootOnUpdate(true);
  httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);  // GitHub assets 302 to a CDN.

  // On success the chip reboots inside update() and we never return.
  const t_httpUpdate_return ret = httpUpdate.update(client, _status.downloadUrl);

  _status.applying = false;
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      _status.lastError = "flash failed: " + httpUpdate.getLastErrorString();
      return false;
    case HTTP_UPDATE_NO_UPDATES:
      _status.lastError = "server reported no update";
      return false;
    case HTTP_UPDATE_OK:
    default:
      return true;  // Unreachable in practice (reboot happens first).
  }
}
