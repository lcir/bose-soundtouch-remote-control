#include "ControlWebServer.h"

#include <WiFi.h>

namespace {
String htmlTemplate(const String& title, const String& body) {
  String html;
  html.reserve(body.length() + 2500);
  html += "<!doctype html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>" + title + "</title>";
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
  html += ".offline{color:var(--danger)}.online{color:var(--ok)}@media (max-width:640px){";
  html += ".controls,.status{grid-template-columns:1fr}}</style></head><body><main>";
  html += body;
  html += "</main></body></html>";
  return html;
}
}  // namespace

ControlWebServer::ControlWebServer(BoseClient& boseClient) : _boseClient(boseClient), _server(80) {
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
  _server.onNotFound([this]() { _server.send(404, "text/plain", "Not found"); });
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
  body += "fetchState().catch(()=>{});setInterval(()=>fetchState().catch(()=>{}),2000);";
  body += "</script>";

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
