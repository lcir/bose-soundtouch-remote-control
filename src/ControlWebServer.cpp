#include "ControlWebServer.h"

#include <WiFi.h>

namespace {
// Inline favicon: a speaker with sound waves in the UI accent colour. Served
// from /favicon.svg so no filesystem/asset partition is needed.
const char kFaviconSvg[] PROGMEM =
    "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'>"
    "<rect width='24' height='24' rx='6' fill='#0f766e'/>"
    "<path d='M5 9.5h3l4-3.2v11.4l-4-3.2H5z' fill='#fff'/>"
    "<path d='M14.8 8.6a4.4 4.4 0 0 1 0 6.8' fill='none' stroke='#fff' stroke-width='1.6' stroke-linecap='round'/>"
    "<path d='M16.9 6.3a7.6 7.6 0 0 1 0 11.4' fill='none' stroke='#fff' stroke-width='1.6' stroke-linecap='round'/>"
    "</svg>";

String htmlTemplate(const String& title, const String& body) {
  String html;
  html.reserve(body.length() + 2500);
  html += "<!doctype html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>" + title + "</title>";
  html += "<link rel='icon' type='image/svg+xml' href='/favicon.svg'>";
  html += "<style>";
  html += ":root{color-scheme:light;--bg:#f4efe6;--panel:#fffaf1;--line:#d8cdb9;--text:#1c1917;";
  html += "--muted:#57534e;--accent:#0f766e;--danger:#b91c1c;--ok:#15803d;}";
  html += "*{box-sizing:border-box}body{margin:0;font-family:ui-sans-serif,system-ui,sans-serif;";
  html += "background:radial-gradient(circle at top,#fff7ed,transparent 35%),var(--bg);color:var(--text);}";
  html += "main{max-width:44rem;margin:0 auto;padding:20px;}section{background:var(--panel);border:1px solid ";
  html += "var(--line);border-radius:20px;padding:18px;margin-bottom:16px;box-shadow:0 12px 32px rgba(0,0,0,.05);}";
  html += "h1{margin:0 0 12px;font-size:1.6rem}h2{margin:0 0 12px;font-size:1rem}p{margin:0;color:var(--muted)}";
  html += ".status{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px;margin-top:14px;}";
  html += ".chip{padding:10px 12px;border-radius:14px;background:#f7f2e7;border:1px solid var(--line);}";
  html += ".value{font-weight:700;color:var(--text);display:block;margin-top:4px}";
  html += ".controls{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px;}";
  html += "button{width:100%;padding:14px 16px;border:0;border-radius:14px;font-weight:700;font-size:1rem;";
  html += "cursor:pointer;background:var(--accent);color:white;}button.secondary{background:#334155}";
  html += "button.danger{background:var(--danger)}button.source{background:#0f766e}";
  html += ".sources{display:grid;grid-template-columns:repeat(auto-fit,minmax(9rem,1fr));gap:10px;}";
  html += ".source-btn{background:#e7f6f4;color:#115e59;border:1px solid #99f6e4}";
  html += ".source-btn.active{background:#115e59;color:#fff;border-color:#115e59}";
  html += ".slider-wrap{display:grid;gap:8px}.slider-row{display:flex;gap:12px;align-items:center}";
  html += "input[type=range]{width:100%}.footer{font-size:.92rem;color:var(--muted);margin-top:10px}";
  html += ".offline{color:var(--danger)}.online{color:var(--ok)}";
  html += ".overlay{position:fixed;inset:0;background:rgba(28,25,23,.85);display:flex;flex-direction:column;";
  html += "align-items:center;justify-content:center;gap:18px;z-index:100;color:#fff;text-align:center;padding:24px;}";
  html += ".overlay[hidden]{display:none}";
  html += ".ring-wrap{position:relative;width:150px;height:150px}";
  html += ".ring-wrap svg{transform:rotate(-90deg);width:150px;height:150px}";
  html += ".ring-bg{fill:none;stroke:rgba(255,255,255,.16);stroke-width:9}";
  html += ".ring-fg{fill:none;stroke:#5eead4;stroke-width:9;stroke-linecap:round;transition:stroke-dashoffset 1s linear}";
  html += ".count{position:absolute;inset:0;display:flex;align-items:center;justify-content:center;font-size:3.2rem;font-weight:800}";
  html += ".overlay p{color:#e7e5e4;font-size:1.05rem;max-width:20rem;margin:0}";
  html += "@media (max-width:640px){";
  html += ".controls,.status{grid-template-columns:1fr}}</style></head><body><main>";
  html += body;
  html += "</main></body></html>";
  return html;
}
}  // namespace

ControlWebServer::ControlWebServer(BoseClient& boseClient, OtaUpdater& ota)
    : _boseClient(boseClient), _ota(ota), _server(80) {
  registerRoutes();
}

void ControlWebServer::start(const DeviceConfig& config) {
  _config = config;
  if (_active) {
    return;
  }
  _server.begin();
  _active = true;
}

void ControlWebServer::stop() {
  if (!_active) {
    return;
  }
  _server.stop();
  _active = false;
}

void ControlWebServer::loop() {
  if (!_active) {
    return;
  }
  _server.handleClient();
}

bool ControlWebServer::isActive() const {
  return _active;
}

void ControlWebServer::registerRoutes() {
  _server.on("/", HTTP_GET, [this]() { handleRoot(); });
  _server.on("/api/state", HTTP_GET, [this]() { handleState(); });
  _server.on("/api/power", HTTP_POST, [this]() { handlePowerAction(); });
  _server.on("/api/source/next", HTTP_POST, [this]() { handleSourceNext(); });
  _server.on("/api/source/select", HTTP_POST, [this]() { handleSourceSelect(); });
  _server.on("/api/standby", HTTP_POST, [this]() { handleStandby(); });
  _server.on("/api/volume", HTTP_POST, [this]() { handleVolume(); });
  _server.on("/api/ota", HTTP_GET, [this]() { handleOtaStatus(); });
  _server.on("/api/ota/check", HTTP_POST, [this]() { handleOtaCheck(); });
  _server.on("/api/ota/apply", HTTP_POST, [this]() { handleOtaApply(); });
  _server.on("/favicon.svg", HTTP_GET, [this]() {
    _server.sendHeader("Cache-Control", "max-age=86400");
    _server.send_P(200, "image/svg+xml", kFaviconSvg);
  });
  _server.onNotFound([this]() { _server.send(404, "text/plain", "Not found"); });
}

bool ControlWebServer::consumeOtaApplyRequest() {
  if (!_otaApplyRequested) {
    return false;
  }
  _otaApplyRequested = false;
  return true;
}

void ControlWebServer::handleRoot() {
  _server.send(200, "text/html; charset=utf-8", buildPage());
}

void ControlWebServer::handleState() {
  sendJsonState();
}

void ControlWebServer::handlePowerAction() {
  const bool wasPoweredOn = _boseClient.state().poweredOn;
  const bool ok = wasPoweredOn ? _boseClient.standby() : _boseClient.wake();
  const String message = wasPoweredOn ? "Standby" : "Wake";
  sendJsonResult(ok, ok ? message : "Unable to change power state");
}

void ControlWebServer::handleSourceNext() {
  String label;
  const bool ok = _boseClient.selectRelativeSource(1, &label);
  sendJsonResult(ok, ok ? label : "Unable to change source");
}

void ControlWebServer::handleSourceSelect() {
  const String id = _server.arg("id");
  const bool ok = !id.isEmpty() && _boseClient.selectSource(id);
  sendJsonResult(ok, ok ? id : "Unable to select source");
}

void ControlWebServer::handleStandby() {
  const bool ok = _boseClient.state().poweredOn && _boseClient.standby();
  sendJsonResult(ok, ok ? "Standby" : "Device already off or unavailable");
}

void ControlWebServer::handleVolume() {
  if (!_server.hasArg("value")) {
    sendJsonResult(false, "Missing volume value");
    return;
  }

  const long parsed = _server.arg("value").toInt();
  const uint8_t value = static_cast<uint8_t>(constrain(parsed, 0L, 100L));
  const bool ok = _boseClient.setVolume(value, _boseClient.state().muted && value > 0, false);
  sendJsonResult(ok, ok ? String(value) : "Unable to set volume");
}

void ControlWebServer::handleOtaStatus() {
  _server.send(200, "application/json; charset=utf-8", buildOtaJson());
}

void ControlWebServer::handleOtaCheck() {
  _ota.checkForUpdate();
  _server.send(200, "application/json; charset=utf-8", buildOtaJson());
}

void ControlWebServer::handleOtaApply() {
  if (!_ota.status().updateAvailable) {
    _server.send(409, "application/json; charset=utf-8",
                 "{\"ok\":false,\"message\":\"No update available\"}");
    return;
  }
  // Acknowledge now; the main loop performs the blocking flash + reboot so this
  // response is actually delivered to the browser.
  _otaApplyRequested = true;
  _server.send(200, "application/json; charset=utf-8",
               "{\"ok\":true,\"message\":\"Update starting, device will reboot\"}");
}

String ControlWebServer::buildOtaJson() const {
  const OtaStatus& s = _ota.status();
  String json = "{";
  json += "\"currentVersion\":\"" + jsonEscape(s.currentVersion) + "\",";
  json += "\"latestVersion\":\"" + jsonEscape(s.latestVersion) + "\",";
  json += "\"updateAvailable\":";
  json += s.updateAvailable ? "true" : "false";
  json += ",\"checked\":";
  json += s.checked ? "true" : "false";
  json += ",\"checking\":";
  json += s.checking ? "true" : "false";
  json += ",\"applying\":";
  json += s.applying ? "true" : "false";
  json += ",\"error\":\"" + jsonEscape(s.lastError) + "\"";
  json += "}";
  return json;
}

void ControlWebServer::sendJsonState() {
  _server.send(200, "application/json; charset=utf-8", buildStateJson());
}

void ControlWebServer::sendJsonResult(bool ok, const String& message) {
  String json = "{\"ok\":";
  json += ok ? "true" : "false";
  json += ",\"message\":\"";
  json += jsonEscape(message);
  json += "\",\"state\":";
  json += buildStateJson();
  json += "}";
  _server.send(ok ? 200 : 409, "application/json; charset=utf-8", json);
}

String ControlWebServer::buildPage() const {
  String body;
  body.reserve(8000);
  body += "<section><h1>Bose SoundTouch Remote</h1><p>Local control page hosted by the ESP32.</p>";
  body += "<div class='status'>";
  body += "<div class='chip'>ESP32 IP<span class='value' id='ip'>-</span></div>";
  body += "<div class='chip'>Bose host<span class='value' id='host'>-</span></div>";
  body += "<div class='chip'>Connection<span class='value' id='conn'>-</span></div>";
  body += "<div class='chip'>Power<span class='value' id='power'>-</span></div>";
  body += "<div class='chip'>Source<span class='value' id='source'>-</span></div>";
  body += "<div class='chip'>Now Playing<span class='value' id='track'>-</span></div>";
  body += "</div></section>";

  body += "<section><h2>Quick Controls</h2><div class='controls'>";
  body += "<button class='source' onclick='postAction(\"/api/source/next\")'>Next Source</button>";
  body += "<button id='powerAction' class='danger' onclick='postAction(\"/api/power\")'>Standby</button>";
  body += "</div>";
  body += "<div class='slider-wrap'><div class='slider-row'><span>Volume</span>";
  body += "<strong id='volumeLabel'>0</strong></div>";
  body += "<input id='volumeSlider' type='range' min='0' max='100' step='1'></div>";
  body += "</section>";

  body += "<section><h2>Available Sources</h2><div class='sources' id='sources'></div>";
  body += "<div class='footer'>This page updates automatically every 2 seconds.</div></section>";

  body += "<section><h2>Firmware</h2><div class='status'>";
  body += "<div class='chip'>Installed<span class='value' id='fwCurrent'>-</span></div>";
  body += "<div class='chip'>Latest<span class='value' id='fwLatest'>-</span></div>";
  body += "</div>";
  body += "<p id='fwMessage' class='footer'>Check GitHub for a newer release.</p>";
  body += "<div class='controls'>";
  body += "<button class='secondary' id='fwCheck' onclick='otaCheck()'>Check for update</button>";
  body += "<button class='danger' id='fwApply' onclick='otaApply()' disabled>Update now</button>";
  body += "</div></section>";

  body += "<div id='otaOverlay' class='overlay' hidden>";
  body += "<div class='ring-wrap'><svg viewBox='0 0 150 150'>";
  body += "<circle class='ring-bg' cx='75' cy='75' r='66'></circle>";
  body += "<circle class='ring-fg' id='otaRing' cx='75' cy='75' r='66'></circle>";
  body += "</svg><div class='count' id='otaCount'>30</div></div>";
  body += "<p id='otaMsg'>Updating firmware &mdash; the device is rebooting. This page reloads automatically.</p></div>";

  body += "<script>";
  body += "const $=id=>document.getElementById(id);let volTimer=null;let lastState=null;";
  body += "function esc(s){return String(s==null?'':s).replace(/[&<>\"']/g,m=>({'&':'&amp;','<':'&lt;','>':'&gt;','\"':'&quot;',\"'\":'&#39;'}[m]));}";
  body += "async function fetchState(){const res=await fetch('/api/state');if(!res.ok)throw new Error('state');";
  body += "const state=await res.json();lastState=state;render(state);}";
  body += "async function postAction(url,body){const res=await fetch(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body||''});";
  body += "const json=await res.json();lastState=json.state||lastState;if(lastState)render(lastState);}";
  body += "function scheduleVolume(value){$('volumeLabel').textContent=value;if(volTimer)clearTimeout(volTimer);";
  body += "volTimer=setTimeout(()=>postAction('/api/volume','value='+encodeURIComponent(value)),120);}";
  body += "function render(state){$('ip').textContent=state.ip||'-';$('host').textContent=state.boseHost||'-';";
  body += "$('conn').innerHTML=state.connected?'<span class=\"online\">Online</span>':'<span class=\"offline\">Offline</span>';";
  body += "$('power').textContent=state.poweredOn?'On':'Standby';$('source').textContent=state.currentSourceLabel||'-';";
  body += "const now=[state.artist,state.track].filter(Boolean).join(' - ');$('track').textContent=now||'-';";
  body += "$('volumeLabel').textContent=state.volume;$('volumeSlider').value=state.volume;";
  body += "const powerBtn=$('powerAction');const wake=!state.poweredOn;";
  body += "powerBtn.textContent=wake?'Wake':'Standby';powerBtn.className=wake?'danger':'source';powerBtn.disabled=!state.connected;";
  body += "const sources=(state.sources||[]).filter(s=>s.visible);let html='';";
  body += "for(const s of sources){const active=s.id===state.currentSourceId?' active':'';const disabled=s.selectable?'':' disabled';";
  body += "const status=(s.ready||s.selectable)?'':' ('+esc(s.status||'')+')';";
  body += "html+='<button class=\"source-btn'+active+'\" data-source=\"'+esc(s.id)+'\"'+disabled+'>'+esc(s.label||s.id)+status+'</button>';}";
  body += "$('sources').innerHTML=html||'<p>No sources available</p>';";
  body += "$('sources').querySelectorAll('button[data-source]').forEach(btn=>btn.onclick=()=>postAction('/api/source/select','id='+encodeURIComponent(btn.dataset.source)));}";
  body += "$('volumeSlider').addEventListener('input',e=>scheduleVolume(e.target.value));";
  body += "function renderOta(o){$('fwCurrent').textContent=o.currentVersion||'-';";
  body += "$('fwLatest').textContent=o.latestVersion||(o.checked?'-':'unknown');";
  body += "const apply=$('fwApply');apply.disabled=!o.updateAvailable||o.applying;";
  body += "$('fwCheck').disabled=o.checking||o.applying;";
  body += "let msg='Check GitHub for a newer release.';";
  body += "if(o.error)msg='Error: '+o.error;else if(o.applying)msg='Updating, device will reboot...';";
  body += "else if(o.updateAvailable)msg='Update '+esc(o.latestVersion)+' available.';";
  body += "else if(o.checked)msg='You are on the latest version.';$('fwMessage').textContent=msg;}";
  body += "async function otaRefresh(){try{const r=await fetch('/api/ota');renderOta(await r.json());}catch(e){}}";
  body += "async function otaCheck(){$('fwMessage').textContent='Checking...';";
  body += "try{const r=await fetch('/api/ota/check',{method:'POST'});renderOta(await r.json());}catch(e){$('fwMessage').textContent='Check failed';}}";
  body += "function startOtaCountdown(secs){const ov=$('otaOverlay');ov.hidden=false;";
  body += "const ring=$('otaRing');const C=2*Math.PI*66;ring.style.strokeDasharray=C;ring.style.strokeDashoffset=0;let r=secs;";
  body += "const tick=()=>{$('otaCount').textContent=r;ring.style.strokeDashoffset=C*(secs-r)/secs;";
  body += "if(r<=0){$('otaMsg').textContent='Reloading...';location.reload();return;}r--;setTimeout(tick,1000);};tick();}";
  body += "async function otaApply(){if(!confirm('Download and flash the new firmware? The device will reboot.'))return;";
  body += "$('fwMessage').textContent='Starting update...';$('fwApply').disabled=true;";
  body += "try{await fetch('/api/ota/apply',{method:'POST'});startOtaCountdown(30);}catch(e){$('fwMessage').textContent='Update request failed';}}";
  body += "fetchState().catch(()=>{});setInterval(()=>fetchState().catch(()=>{}),2000);";
  body += "otaRefresh();</script>";

  return htmlTemplate("Bose Remote", body);
}

String ControlWebServer::buildStateJson() const {
  const BoseState& state = _boseClient.state();
  const auto& sources = _boseClient.sources();
  std::vector<BoseSource> groupedSources;
  groupedSources.reserve(sources.size());
  for (const auto& source : sources) {
    const String selectionId = _boseClient.selectionIdForSource(source);
    if (selectionId.isEmpty()) {
      continue;
    }

    int existingIndex = -1;
    for (size_t i = 0; i < groupedSources.size(); ++i) {
      if (groupedSources[i].id == selectionId) {
        existingIndex = static_cast<int>(i);
        break;
      }
    }

    if (existingIndex < 0) {
      BoseSource grouped = source;
      grouped.id = selectionId;
      grouped.label = _boseClient.selectionLabelForSource(source);
      grouped.visible = source.visible;
      grouped.selectable = source.selectable;
      grouped.ready = source.ready;
      groupedSources.push_back(grouped);
      continue;
    }

    BoseSource& grouped = groupedSources[existingIndex];
    grouped.visible = grouped.visible || source.visible;
    grouped.selectable = grouped.selectable || source.selectable;
    grouped.ready = grouped.ready || source.ready;
    if (grouped.ready) {
      grouped.status = "READY";
    } else if (grouped.status.isEmpty()) {
      grouped.status = source.status;
    }
  }

  String json = "{";
  json += "\"ip\":\"" + jsonEscape(WiFi.localIP().toString()) + "\",";
  json += "\"boseHost\":\"" + jsonEscape(_config.boseHost) + "\",";
  json += "\"connected\":";
  json += state.connected ? "true" : "false";
  json += ",\"wsConnected\":";
  json += state.wsConnected ? "true" : "false";
  json += ",\"poweredOn\":";
  json += state.poweredOn ? "true" : "false";
  json += ",\"volume\":";
  json += String(state.volume);
  json += ",\"muted\":";
  json += state.muted ? "true" : "false";
  json += ",\"currentSourceId\":\"" + jsonEscape(_boseClient.currentSelectionId()) + "\",";
  json += "\"currentSourceLabel\":\"" + jsonEscape(state.currentSourceLabel) + "\",";
  json += "\"track\":\"" + jsonEscape(state.track) + "\",";
  json += "\"artist\":\"" + jsonEscape(state.artist) + "\",";
  json += "\"sources\":[";
  for (size_t i = 0; i < groupedSources.size(); ++i) {
    if (i > 0) {
      json += ",";
    }
    json += "{";
    json += "\"id\":\"" + jsonEscape(groupedSources[i].id) + "\",";
    json += "\"label\":\"" + jsonEscape(groupedSources[i].label) + "\",";
    json += "\"status\":\"" + jsonEscape(groupedSources[i].status) + "\",";
    json += "\"ready\":";
    json += groupedSources[i].ready ? "true" : "false";
    json += ",\"visible\":";
    json += groupedSources[i].visible ? "true" : "false";
    json += ",\"selectable\":";
    json += groupedSources[i].selectable ? "true" : "false";
    json += "}";
  }
  json += "]}";
  return json;
}

String ControlWebServer::jsonEscape(const String& value) {
  String escaped;
  escaped.reserve(value.length() + 8);
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value[i];
    switch (c) {
      case '\\':
        escaped += "\\\\";
        break;
      case '"':
        escaped += "\\\"";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped += c;
        break;
    }
  }
  return escaped;
}
