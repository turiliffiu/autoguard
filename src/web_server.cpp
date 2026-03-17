// ============================================================
// AutoGuard - Web Server + Dashboard - Implementazione
// ============================================================
#include "web_server.h"

AutoGuardWeb::AutoGuardWeb(AlarmLogic& alarmSys, SensorLD2420& radar) :
    _server(WEB_SERVER_PORT),
    _alarmSys(alarmSys),
    _radar(radar),
    _wifiConnected(false)
{}

bool AutoGuardWeb::begin() {
    if (!_connectWiFi()) return false;
    _setupRoutes();
    _server.begin();
    Serial.printf("[WEB] Server avviato su http://%s\n",
        WiFi.localIP().toString().c_str());
    return true;
}

void AutoGuardWeb::update() {
    if (WiFi.status() != WL_CONNECTED) {
        if (_wifiConnected) {
            Serial.println("[WEB] WiFi disconnesso! Riconnessione...");
            _wifiConnected = false;
        }
        _connectWiFi();
    }
}

bool AutoGuardWeb::_connectWiFi() {
    Serial.printf("[WEB] Connessione a WiFi: %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(WIFI_HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > WIFI_TIMEOUT_MS) {
            Serial.println("[WEB] ERRORE: WiFi timeout!");
            return false;
        }
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.printf("[WEB] WiFi connesso! IP: %s\n",
        WiFi.localIP().toString().c_str());
    Serial.printf("[WEB] RSSI: %d dBm\n", WiFi.RSSI());
    _wifiConnected = true;
    return true;
}

void AutoGuardWeb::_setupRoutes() {
    _server.on("/", HTTP_GET, [this](AsyncWebServerRequest* req) {
        req->send(200, "text/html", _buildDashboardHtml());
    });

    _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
        req->send(200, "application/json", _buildStatusJson());
    });

    _server.on("/api/arm", HTTP_POST, [this](AsyncWebServerRequest* req) {
        _alarmSys.arm();
        req->send(200, "application/json", "{\"ok\":true,\"cmd\":\"arm\"}");
    });

    _server.on("/api/disarm", HTTP_POST, [this](AsyncWebServerRequest* req) {
        _alarmSys.disarm();
        req->send(200, "application/json", "{\"ok\":true,\"cmd\":\"disarm\"}");
    });

    _server.on("/api/reset", HTTP_POST, [this](AsyncWebServerRequest* req) {
        _alarmSys.reset();
        req->send(200, "application/json", "{\"ok\":true,\"cmd\":\"reset\"}");
    });

    _server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest* req) {
        AutoGuardConfig cfg = configMgr.get();
        JsonDocument doc;
        doc["zoneCriticalMax"]   = cfg.zoneCriticalMax;
        doc["zoneMediumMax"]     = cfg.zoneMediumMax;
        doc["zoneFarMax"]        = cfg.zoneFarMax;
        doc["armingDelayMs"]     = cfg.armingDelayMs;
        doc["preAlarmMs"]        = cfg.preAlarmMs;
        doc["alarmDurationMs"]   = cfg.alarmDurationMs;
        doc["cooldownMs"]        = cfg.cooldownMs;
        doc["detectionsToAlert"] = cfg.detectionsToAlert;
        doc["radarMinDist"]      = cfg.radarMinDist;
        doc["radarMaxDist"]      = cfg.radarMaxDist;
        String json;
        serializeJson(doc, json);
        req->send(200, "application/json", json);
    });

    _server.on("/api/config", HTTP_POST,
        [](AsyncWebServerRequest* req) {},
        nullptr,
        [this](AsyncWebServerRequest* req, uint8_t* data, size_t len,
               size_t index, size_t total) {
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, data, len);
            if (err) {
                req->send(400, "application/json", "{\"error\":\"JSON invalido\"}");
                return;
            }
            AutoGuardConfig cfg = configMgr.get();
            if (doc["zoneCriticalMax"].is<int>())  cfg.zoneCriticalMax   = doc["zoneCriticalMax"];
            if (doc["zoneMediumMax"].is<int>())     cfg.zoneMediumMax     = doc["zoneMediumMax"];
            if (doc["zoneFarMax"].is<int>())        cfg.zoneFarMax        = doc["zoneFarMax"];
            if (doc["armingDelayMs"].is<int>())     cfg.armingDelayMs     = doc["armingDelayMs"];
            if (doc["preAlarmMs"].is<int>())        cfg.preAlarmMs        = doc["preAlarmMs"];
            if (doc["alarmDurationMs"].is<int>())   cfg.alarmDurationMs   = doc["alarmDurationMs"];
            if (doc["cooldownMs"].is<int>())        cfg.cooldownMs        = doc["cooldownMs"];
            if (doc["detectionsToAlert"].is<int>()) cfg.detectionsToAlert = doc["detectionsToAlert"];
            if (doc["radarMinDist"].is<int>())      cfg.radarMinDist      = doc["radarMinDist"];
            if (doc["radarMaxDist"].is<int>())      cfg.radarMaxDist      = doc["radarMaxDist"];
            configMgr.save(cfg);
            req->send(200, "application/json", "{\"ok\":true}");
        }
    );

    _server.on("/api/config/reset", HTTP_POST, [](AsyncWebServerRequest* req) {
        configMgr.resetDefaults();
        req->send(200, "application/json", "{\"ok\":true}");
    });

    _server.on("/config", HTTP_GET, [this](AsyncWebServerRequest* req) {
        req->send(200, "text/html", _buildConfigHtml());
    });

    _server.onNotFound([](AsyncWebServerRequest* req) {
        req->send(404, "application/json", "{\"error\":\"not found\"}");
    });
}

String AutoGuardWeb::_buildStatusJson() {
    JsonDocument doc;
    RadarData radarData = _radar.getData();
    doc["state"]        = _alarmSys.getStateName();
    doc["state_id"]     = (int)_alarmSys.getState();
    doc["elapsed_ms"]   = _alarmSys.getStateElapsedMs();
    doc["arming_ms"]    = _alarmSys.getArmingCountdown();
    doc["alarm_ms"]     = _alarmSys.getAlarmElapsedMs();
    JsonObject radar = doc["radar"].to<JsonObject>();
    radar["detected"]   = radarData.detected;
    radar["distance"]   = radarData.filtered_dist;
    radar["zone"]       = (int)radarData.zone;
    radar["raw_dist"]   = radarData.distance_cm;
    JsonObject wifi = doc["wifi"].to<JsonObject>();
    wifi["ssid"]        = WiFi.SSID();
    wifi["ip"]          = WiFi.localIP().toString();
    wifi["rssi"]        = WiFi.RSSI();
    doc["uptime_s"]     = millis() / 1000;
    doc["free_heap"]    = ESP.getFreeHeap();
    doc["fw_version"]   = FW_VERSION;
    String out;
    serializeJson(doc, out);
    return out;
}

bool AutoGuardWeb::isConnected() {
    return _wifiConnected && WiFi.status() == WL_CONNECTED;
}

String AutoGuardWeb::getIP() {
    return WiFi.localIP().toString();
}

// ============================================================
// _buildDashboardHtml()
// ============================================================
String AutoGuardWeb::_buildDashboardHtml() {
    return R"rawhtml(
<!DOCTYPE html>
<html lang="it">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>AutoGuard - Antifurto Auto</title>
<style>
  * { margin:0; padding:0; box-sizing:border-box; }
  body { font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif; background:#0f172a; color:#e2e8f0; min-height:100vh; padding:20px; }
  .header { text-align:center; padding:30px 0 20px; }
  .header h1 { font-size:2em; color:#f1f5f9; }
  .header p { color:#94a3b8; margin-top:5px; }
  .nav { text-align:center; margin-bottom:20px; }
  .nav a { color:#60a5fa; text-decoration:none; margin:0 10px; font-size:0.9em; }
  .nav a:hover { color:#93c5fd; }
  .grid { display:grid; grid-template-columns:repeat(auto-fit,minmax(280px,1fr)); gap:20px; max-width:1000px; margin:0 auto; }
  .card { background:#1e293b; border-radius:16px; padding:25px; border:1px solid #334155; }
  .card h2 { font-size:0.85em; text-transform:uppercase; letter-spacing:1px; color:#64748b; margin-bottom:15px; }
  .state-badge { display:inline-block; padding:8px 20px; border-radius:50px; font-size:1.3em; font-weight:700; letter-spacing:1px; margin-bottom:10px; }
  .state-DISARMED  { background:#1e3a5f; color:#60a5fa; }
  .state-ARMING    { background:#3b2f00; color:#fbbf24; }
  .state-ARMED     { background:#14432a; color:#34d399; }
  .state-ALERT     { background:#3b1f00; color:#fb923c; }
  .state-ALARM     { background:#4b0000; color:#f87171; animation:pulse 0.5s infinite; }
  .state-COOLDOWN  { background:#2d1b69; color:#a78bfa; }
  @keyframes pulse { 0%,100%{opacity:1} 50%{opacity:0.5} }
  .elapsed { color:#64748b; font-size:0.9em; margin-top:5px; }
  .radar-value { font-size:3em; font-weight:700; color:#f1f5f9; margin:10px 0; }
  .radar-unit { font-size:0.4em; color:#64748b; }
  .zone-bar { height:8px; border-radius:4px; background:#334155; margin:10px 0; overflow:hidden; }
  .zone-fill { height:100%; border-radius:4px; transition:width 0.3s,background 0.3s; }
  .zone-label { font-size:0.85em; color:#94a3b8; }
  .btn-group { display:flex; gap:10px; flex-wrap:wrap; }
  .btn { flex:1; padding:14px; border:none; border-radius:10px; font-size:1em; font-weight:600; cursor:pointer; transition:all 0.2s; min-width:80px; }
  .btn:hover { transform:translateY(-2px); opacity:0.9; }
  .btn-arm    { background:#059669; color:white; }
  .btn-disarm { background:#2563eb; color:white; }
  .btn-reset  { background:#d97706; color:white; }
  .info-row { display:flex; justify-content:space-between; padding:8px 0; border-bottom:1px solid #334155; font-size:0.9em; }
  .info-row:last-child { border-bottom:none; }
  .info-label { color:#64748b; }
  .info-value { color:#e2e8f0; font-weight:500; }
  .dot { display:inline-block; width:12px; height:12px; border-radius:50%; margin-right:6px; }
  .dot-on  { background:#22c55e; box-shadow:0 0 8px #22c55e; }
  .dot-off { background:#374151; }
  .footer { text-align:center; margin-top:30px; color:#475569; font-size:0.8em; }
  .config-link { text-align:center; margin-top:15px; }
  .config-link a { color:#60a5fa; text-decoration:none; font-size:0.85em; }
</style>
</head>
<body>
<div class="header">
  <h1>AutoGuard</h1>
  <p>Antifurto Auto ESP32-C6 + HLK-LD2420</p>
</div>
<div class="nav">
  <a href="/">Dashboard</a>
  <a href="/config">⚙️ Configurazione</a>
</div>
<div class="grid">
  <div class="card">
    <h2>Stato Sistema</h2>
    <div class="state-badge state-DISARMED" id="stateBadge">DISARMED</div>
    <div class="elapsed" id="stateElapsed">0s nello stato corrente</div>
    <div id="armingInfo" style="display:none;margin-top:10px;color:#fbbf24;">
      Armamento in <span id="armingCountdown">5</span>s...
    </div>
  </div>
  <div class="card">
    <h2>Sensore Radar</h2>
    <div><span class="dot dot-off" id="detDot"></span><span id="detLabel" style="color:#94a3b8;">Nessuna presenza</span></div>
    <div class="radar-value"><span id="radarDist">--</span><span class="radar-unit">cm</span></div>
    <div class="zone-bar"><div class="zone-fill" id="zoneFill" style="width:0%;background:#334155;"></div></div>
    <div class="zone-label" id="zoneLabel">Zona: --</div>
  </div>
  <div class="card">
    <h2>Controlli</h2>
    <div class="btn-group">
      <button class="btn btn-arm"    onclick="sendCmd('arm')">ARM</button>
      <button class="btn btn-disarm" onclick="sendCmd('disarm')">DISARM</button>
      <button class="btn btn-reset"  onclick="sendCmd('reset')">RESET</button>
    </div>
    <div id="cmdFeedback" style="margin-top:12px;font-size:0.85em;color:#64748b;"></div>
  </div>
  <div class="card">
    <h2>Info Sistema</h2>
    <div class="info-row"><span class="info-label">IP</span><span class="info-value" id="infoIP">--</span></div>
    <div class="info-row"><span class="info-label">WiFi RSSI</span><span class="info-value" id="infoRSSI">-- dBm</span></div>
    <div class="info-row"><span class="info-label">Uptime</span><span class="info-value" id="infoUptime">--</span></div>
    <div class="info-row"><span class="info-label">RAM libera</span><span class="info-value" id="infoHeap">-- KB</span></div>
    <div class="info-row"><span class="info-label">Firmware</span><span class="info-value" id="infoFW">--</span></div>
  </div>
</div>
<div class="config-link"><a href="/config">⚙️ Configura zone e timing</a></div>
<div class="footer">AutoGuard v1.0.0 &mdash; Aggiornamento ogni 2s</div>
<script>
const zoneNames  = ["--","CRITICA","MEDIA","LONTANA"];
const zoneColors = ["#334155","#ef4444","#f97316","#eab308"];
const zonePct    = [0,100,60,30];
async function fetchStatus() {
  try {
    const r = await fetch("/api/status");
    const d = await r.json();
    const badge = document.getElementById("stateBadge");
    badge.textContent = d.state;
    badge.className = "state-badge state-" + d.state;
    document.getElementById("stateElapsed").textContent = Math.floor(d.elapsed_ms/1000) + "s nello stato corrente";
    const armInfo = document.getElementById("armingInfo");
    if (d.state === "ARMING" && d.arming_ms > 0) {
      armInfo.style.display = "block";
      document.getElementById("armingCountdown").textContent = Math.ceil(d.arming_ms/1000);
    } else { armInfo.style.display = "none"; }
    const det = d.radar.detected;
    document.getElementById("detDot").className = "dot " + (det ? "dot-on" : "dot-off");
    document.getElementById("detLabel").textContent = det ? "Presenza rilevata!" : "Nessuna presenza";
    document.getElementById("detLabel").style.color = det ? "#22c55e" : "#94a3b8";
    document.getElementById("radarDist").textContent = det ? d.radar.distance : "--";
    const zone = d.radar.zone;
    document.getElementById("zoneFill").style.width = zonePct[zone] + "%";
    document.getElementById("zoneFill").style.background = zoneColors[zone];
    document.getElementById("zoneLabel").textContent = "Zona: " + zoneNames[zone];
    document.getElementById("infoIP").textContent    = d.wifi.ip;
    document.getElementById("infoRSSI").textContent  = d.wifi.rssi + " dBm";
    document.getElementById("infoUptime").textContent = formatUptime(d.uptime_s);
    document.getElementById("infoHeap").textContent  = Math.round(d.free_heap/1024) + " KB";
    document.getElementById("infoFW").textContent    = d.fw_version;
  } catch(e) { console.error("Errore:", e); }
}
function formatUptime(s) {
  return Math.floor(s/3600) + "h " + Math.floor((s%3600)/60) + "m " + (s%60) + "s";
}
async function sendCmd(cmd) {
  const fb = document.getElementById("cmdFeedback");
  try {
    fb.textContent = "Invio " + cmd.toUpperCase() + "...";
    await fetch("/api/" + cmd, {method:"POST"});
    fb.textContent = "Comando " + cmd.toUpperCase() + " inviato!";
    fb.style.color = "#34d399";
    fetchStatus();
  } catch(e) { fb.textContent = "Errore!"; fb.style.color = "#f87171"; }
}
setInterval(fetchStatus, 2000);
fetchStatus();
</script>
</body>
</html>
)rawhtml";
}

// ============================================================
// _buildConfigHtml()
// ============================================================
String AutoGuardWeb::_buildConfigHtml() {
    return R"rawhtml(
<!DOCTYPE html>
<html lang="it">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>AutoGuard - Configurazione</title>
<style>
  * { margin:0; padding:0; box-sizing:border-box; }
  body { font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif; background:#0f172a; color:#e2e8f0; min-height:100vh; padding:20px; }
  .header { text-align:center; padding:30px 0 20px; }
  .header h1 { font-size:2em; color:#f1f5f9; }
  .header p  { color:#94a3b8; margin-top:5px; }
  .back { display:inline-block; margin-bottom:20px; color:#60a5fa; text-decoration:none; font-size:0.9em; }
  .grid { display:grid; grid-template-columns:repeat(auto-fit,minmax(300px,1fr)); gap:20px; max-width:900px; margin:0 auto; }
  .card { background:#1e293b; border-radius:16px; padding:25px; border:1px solid #334155; }
  .card h2 { font-size:0.85em; text-transform:uppercase; letter-spacing:1px; color:#64748b; margin-bottom:15px; }
  .field { margin-bottom:15px; }
  .field label { display:block; font-size:0.85em; color:#94a3b8; margin-bottom:5px; }
  .field input { width:100%; padding:10px 12px; background:#0f172a; border:1px solid #334155; border-radius:8px; color:#e2e8f0; font-size:1em; }
  .field input:focus { outline:none; border-color:#60a5fa; }
  .field .unit { font-size:0.75em; color:#475569; margin-top:3px; }
  .btn-group { display:flex; gap:10px; margin-top:20px; }
  .btn { flex:1; padding:12px; border:none; border-radius:10px; font-size:0.95em; font-weight:600; cursor:pointer; transition:all 0.2s; }
  .btn:hover { transform:translateY(-2px); opacity:0.9; }
  .btn-save  { background:#059669; color:white; }
  .btn-reset { background:#dc2626; color:white; }
  .btn-dash  { background:#2563eb; color:white; }
  #feedback { max-width:900px; margin:15px auto 0; padding:12px 20px; border-radius:10px; text-align:center; display:none; }
  .fb-ok  { background:#064e3b; color:#34d399; }
  .fb-err { background:#450a0a; color:#f87171; }
</style>
</head>
<body>
<div class="header"><h1>⚙️ Configurazione</h1><p>Parametri salvati in memoria flash (NVS)</p></div>
<div style="max-width:900px;margin:0 auto;"><a class="back" href="/">← Dashboard</a></div>
<div id="feedback"></div>
<div class="grid">
  <div class="card">
    <h2>📡 Zone Radar</h2>
    <div class="field"><label>Zona CRITICA (max cm)</label><input type="number" id="zoneCriticalMax" min="20" max="400"><div class="unit">default: 100cm</div></div>
    <div class="field"><label>Zona MEDIA (max cm)</label><input type="number" id="zoneMediumMax" min="20" max="400"><div class="unit">default: 250cm</div></div>
    <div class="field"><label>Zona LONTANA (max cm)</label><input type="number" id="zoneFarMax" min="20" max="800"><div class="unit">default: 400cm</div></div>
    <div class="field"><label>Distanza minima radar (cm)</label><input type="number" id="radarMinDist" min="10" max="100"><div class="unit">default: 20cm</div></div>
    <div class="field"><label>Distanza massima radar (cm)</label><input type="number" id="radarMaxDist" min="100" max="800"><div class="unit">default: 400cm</div></div>
  </div>
  <div class="card">
    <h2>⏱️ Timing Allarme</h2>
    <div class="field"><label>Ritardo armamento (ms)</label><input type="number" id="armingDelayMs" min="1000" max="30000" step="1000"><div class="unit">default: 5000ms</div></div>
    <div class="field"><label>Pre-allarme (ms)</label><input type="number" id="preAlarmMs" min="500" max="10000" step="500"><div class="unit">default: 3000ms</div></div>
    <div class="field"><label>Durata allarme (ms)</label><input type="number" id="alarmDurationMs" min="5000" max="120000" step="1000"><div class="unit">default: 30000ms</div></div>
    <div class="field"><label>Cooldown (ms)</label><input type="number" id="cooldownMs" min="1000" max="60000" step="1000"><div class="unit">default: 10000ms</div></div>
    <div class="field"><label>Rilevamenti per alert</label><input type="number" id="detectionsToAlert" min="1" max="50"><div class="unit">default: 5</div></div>
  </div>
</div>
<div style="max-width:900px;margin:20px auto;">
  <div class="btn-group">
    <button class="btn btn-save"  onclick="saveConfig()">💾 Salva</button>
    <button class="btn btn-reset" onclick="resetConfig()">🔄 Reset Default</button>
    <button class="btn btn-dash"  onclick="location.href='/'">🏠 Dashboard</button>
  </div>
</div>
<script>
async function loadConfig() {
  try {
    const r = await fetch("/api/config");
    const d = await r.json();
    document.getElementById("zoneCriticalMax").value   = d.zoneCriticalMax;
    document.getElementById("zoneMediumMax").value     = d.zoneMediumMax;
    document.getElementById("zoneFarMax").value        = d.zoneFarMax;
    document.getElementById("armingDelayMs").value     = d.armingDelayMs;
    document.getElementById("preAlarmMs").value        = d.preAlarmMs;
    document.getElementById("alarmDurationMs").value   = d.alarmDurationMs;
    document.getElementById("cooldownMs").value        = d.cooldownMs;
    document.getElementById("detectionsToAlert").value = d.detectionsToAlert;
    document.getElementById("radarMinDist").value      = d.radarMinDist;
    document.getElementById("radarMaxDist").value      = d.radarMaxDist;
  } catch(e) { showFeedback("Errore caricamento!", false); }
}
async function saveConfig() {
  const cfg = {
    zoneCriticalMax:   parseInt(document.getElementById("zoneCriticalMax").value),
    zoneMediumMax:     parseInt(document.getElementById("zoneMediumMax").value),
    zoneFarMax:        parseInt(document.getElementById("zoneFarMax").value),
    armingDelayMs:     parseInt(document.getElementById("armingDelayMs").value),
    preAlarmMs:        parseInt(document.getElementById("preAlarmMs").value),
    alarmDurationMs:   parseInt(document.getElementById("alarmDurationMs").value),
    cooldownMs:        parseInt(document.getElementById("cooldownMs").value),
    detectionsToAlert: parseInt(document.getElementById("detectionsToAlert").value),
    radarMinDist:      parseInt(document.getElementById("radarMinDist").value),
    radarMaxDist:      parseInt(document.getElementById("radarMaxDist").value),
  };
  try {
    const r = await fetch("/api/config", {method:"POST", headers:{"Content-Type":"application/json"}, body:JSON.stringify(cfg)});
    const d = await r.json();
    showFeedback(d.ok ? "✅ Configurazione salvata!" : "❌ Errore!", d.ok);
  } catch(e) { showFeedback("❌ Errore connessione!", false); }
}
async function resetConfig() {
  if (!confirm("Reset ai valori di default?")) return;
  try {
    await fetch("/api/config/reset", {method:"POST"});
    showFeedback("🔄 Reset eseguito!", true);
    loadConfig();
  } catch(e) { showFeedback("❌ Errore!", false); }
}
function showFeedback(msg, ok) {
  const fb = document.getElementById("feedback");
  fb.textContent = msg;
  fb.className = ok ? "fb-ok" : "fb-err";
  fb.style.display = "block";
  setTimeout(() => fb.style.display = "none", 3000);
}
loadConfig();
</script>
</body>
</html>
)rawhtml";
}
