// ============================================================
// AutoGuard - Web Server + Dashboard - Implementazione
// ============================================================
#include "web_server.h"

// ============================================================
// Costruttore
// ============================================================
AutoGuardWeb::AutoGuardWeb(AlarmLogic& alarmSys, SensorLD2420& radar) :
    _server(WEB_SERVER_PORT),
    _alarmSys(alarmSys),
    _radar(radar),
    _wifiConnected(false)
{}

// ============================================================
// begin() - Connetti WiFi e avvia server
// ============================================================
bool AutoGuardWeb::begin() {
    if (!_connectWiFi()) return false;
    _setupRoutes();
    _server.begin();
    Serial.printf("[WEB] Server avviato su http://%s\n",
        WiFi.localIP().toString().c_str());
    return true;
}

// ============================================================
// update() - Gestione riconnessione WiFi
// ============================================================
void AutoGuardWeb::update() {
    if (WiFi.status() != WL_CONNECTED) {
        if (_wifiConnected) {
            Serial.println("[WEB] WiFi disconnesso! Riconnessione...");
            _wifiConnected = false;
        }
        _connectWiFi();
    }
}

// ============================================================
// _connectWiFi()
// ============================================================
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

// ============================================================
// _setupRoutes() - Definisce le API e la dashboard
// ============================================================
void AutoGuardWeb::_setupRoutes() {

    // --- GET / → Dashboard HTML ---
    _server.on("/", HTTP_GET, [this](AsyncWebServerRequest* req) {
        req->send(200, "text/html", _buildDashboardHtml());
    });

    // --- GET /api/status → JSON stato sistema ---
    _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
        req->send(200, "application/json", _buildStatusJson());
    });

    // --- POST /api/arm → Arma sistema ---
    _server.on("/api/arm", HTTP_POST, [this](AsyncWebServerRequest* req) {
        _alarmSys.arm();
        req->send(200, "application/json",
            "{\"ok\":true,\"cmd\":\"arm\"}");
    });

    // --- POST /api/disarm → Disarma sistema ---
    _server.on("/api/disarm", HTTP_POST, [this](AsyncWebServerRequest* req) {
        _alarmSys.disarm();
        req->send(200, "application/json",
            "{\"ok\":true,\"cmd\":\"disarm\"}");
    });

    // --- POST /api/reset → Reset allarme ---
    _server.on("/api/reset", HTTP_POST, [this](AsyncWebServerRequest* req) {
        _alarmSys.reset();
        req->send(200, "application/json",
            "{\"ok\":true,\"cmd\":\"reset\"}");
    });

    // --- 404 ---
    _server.onNotFound([](AsyncWebServerRequest* req) {
        req->send(404, "application/json", "{\"error\":\"not found\"}");
    });
}

// ============================================================
// _buildStatusJson() - JSON con stato completo
// ============================================================
String AutoGuardWeb::_buildStatusJson() {
    JsonDocument doc;
    RadarData radarData = _radar.getData();

    // Stato sistema
    doc["state"]        = _alarmSys.getStateName();
    doc["state_id"]     = (int)_alarmSys.getState();
    doc["elapsed_ms"]   = _alarmSys.getStateElapsedMs();
    doc["arming_ms"]    = _alarmSys.getArmingCountdown();
    doc["alarm_ms"]     = _alarmSys.getAlarmElapsedMs();

    // Dati radar
    JsonObject radar = doc["radar"].to<JsonObject>();
    radar["detected"]   = radarData.detected;
    radar["distance"]   = radarData.filtered_dist;
    radar["zone"]       = (int)radarData.zone;
    radar["raw_dist"]   = radarData.distance_cm;

    // WiFi
    JsonObject wifi = doc["wifi"].to<JsonObject>();
    wifi["ssid"]        = WiFi.SSID();
    wifi["ip"]          = WiFi.localIP().toString();
    wifi["rssi"]        = WiFi.RSSI();

    // Sistema
    doc["uptime_s"]     = millis() / 1000;
    doc["free_heap"]    = ESP.getFreeHeap();
    doc["fw_version"]   = FW_VERSION;

    String out;
    serializeJson(doc, out);
    return out;
}

// ============================================================
// _buildDashboardHtml() - Dashboard HTML completa
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
  body {
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
    background: #0f172a;
    color: #e2e8f0;
    min-height: 100vh;
    padding: 20px;
  }
  .header {
    text-align: center;
    padding: 30px 0 20px;
  }
  .header h1 { font-size: 2em; color: #f1f5f9; }
  .header p  { color: #94a3b8; margin-top: 5px; }
  .grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
    gap: 20px;
    max-width: 1000px;
    margin: 0 auto;
  }
  .card {
    background: #1e293b;
    border-radius: 16px;
    padding: 25px;
    border: 1px solid #334155;
  }
  .card h2 {
    font-size: 0.85em;
    text-transform: uppercase;
    letter-spacing: 1px;
    color: #64748b;
    margin-bottom: 15px;
  }
  /* Stato sistema */
  .state-badge {
    display: inline-block;
    padding: 8px 20px;
    border-radius: 50px;
    font-size: 1.3em;
    font-weight: 700;
    letter-spacing: 1px;
    margin-bottom: 10px;
  }
  .state-DISARMED  { background:#1e3a5f; color:#60a5fa; }
  .state-ARMING    { background:#3b2f00; color:#fbbf24; }
  .state-ARMED     { background:#14432a; color:#34d399; }
  .state-ALERT     { background:#3b1f00; color:#fb923c; }
  .state-ALARM     { background:#4b0000; color:#f87171; animation: pulse 0.5s infinite; }
  .state-COOLDOWN  { background:#2d1b69; color:#a78bfa; }
  @keyframes pulse {
    0%,100% { opacity:1; }
    50%      { opacity:0.5; }
  }
  .elapsed { color:#64748b; font-size:0.9em; margin-top:5px; }
  /* Radar */
  .radar-value {
    font-size: 3em;
    font-weight: 700;
    color: #f1f5f9;
    margin: 10px 0;
  }
  .radar-unit { font-size:0.4em; color:#64748b; }
  .zone-bar {
    height: 8px;
    border-radius: 4px;
    background: #334155;
    margin: 10px 0;
    overflow: hidden;
  }
  .zone-fill {
    height: 100%;
    border-radius: 4px;
    transition: width 0.3s, background 0.3s;
  }
  .zone-label { font-size:0.85em; color:#94a3b8; }
  /* Controlli */
  .btn-group { display:flex; gap:10px; flex-wrap:wrap; }
  .btn {
    flex: 1;
    padding: 14px;
    border: none;
    border-radius: 10px;
    font-size: 1em;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.2s;
    min-width: 80px;
  }
  .btn:hover { transform: translateY(-2px); opacity:0.9; }
  .btn:active { transform: translateY(0); }
  .btn-arm     { background:#059669; color:white; }
  .btn-disarm  { background:#2563eb; color:white; }
  .btn-reset   { background:#d97706; color:white; }
  /* Info */
  .info-row {
    display: flex;
    justify-content: space-between;
    padding: 8px 0;
    border-bottom: 1px solid #334155;
    font-size: 0.9em;
  }
  .info-row:last-child { border-bottom: none; }
  .info-label { color:#64748b; }
  .info-value { color:#e2e8f0; font-weight:500; }
  /* Detection dot */
  .dot {
    display: inline-block;
    width: 12px; height: 12px;
    border-radius: 50%;
    margin-right: 6px;
  }
  .dot-on  { background:#22c55e; box-shadow:0 0 8px #22c55e; }
  .dot-off { background:#374151; }
  /* Footer */
  .footer {
    text-align: center;
    margin-top: 30px;
    color: #475569;
    font-size: 0.8em;
  }
</style>
</head>
<body>

<div class="header">
  <h1>AutoGuard</h1>
  <p>Antifurto Auto ESP32-C6 + HLK-LD2420</p>
</div>

<div class="grid">

  <!-- Card Stato Sistema -->
  <div class="card">
    <h2>Stato Sistema</h2>
    <div class="state-badge state-DISARMED" id="stateBadge">DISARMED</div>
    <div class="elapsed" id="stateElapsed">0s nello stato corrente</div>
    <div id="armingInfo" style="display:none; margin-top:10px; color:#fbbf24;">
      Armamento in <span id="armingCountdown">5</span>s...
    </div>
  </div>

  <!-- Card Radar -->
  <div class="card">
    <h2>Sensore Radar</h2>
    <div>
      <span class="dot dot-off" id="detDot"></span>
      <span id="detLabel" style="color:#94a3b8;">Nessuna presenza</span>
    </div>
    <div class="radar-value">
      <span id="radarDist">--</span>
      <span class="radar-unit">cm</span>
    </div>
    <div class="zone-bar">
      <div class="zone-fill" id="zoneFill" style="width:0%; background:#334155;"></div>
    </div>
    <div class="zone-label" id="zoneLabel">Zona: --</div>
  </div>

  <!-- Card Controlli -->
  <div class="card">
    <h2>Controlli</h2>
    <div class="btn-group">
      <button class="btn btn-arm"    onclick="sendCmd('arm')">ARM</button>
      <button class="btn btn-disarm" onclick="sendCmd('disarm')">DISARM</button>
      <button class="btn btn-reset"  onclick="sendCmd('reset')">RESET</button>
    </div>
    <div id="cmdFeedback" style="margin-top:12px; font-size:0.85em; color:#64748b;"></div>
  </div>

  <!-- Card Info Sistema -->
  <div class="card">
    <h2>Info Sistema</h2>
    <div class="info-row">
      <span class="info-label">IP</span>
      <span class="info-value" id="infoIP">--</span>
    </div>
    <div class="info-row">
      <span class="info-label">WiFi RSSI</span>
      <span class="info-value" id="infoRSSI">-- dBm</span>
    </div>
    <div class="info-row">
      <span class="info-label">Uptime</span>
      <span class="info-value" id="infoUptime">--</span>
    </div>
    <div class="info-row">
      <span class="info-label">RAM libera</span>
      <span class="info-value" id="infoHeap">-- KB</span>
    </div>
    <div class="info-row">
      <span class="info-label">Firmware</span>
      <span class="info-value" id="infoFW">--</span>
    </div>
  </div>

</div>

<div class="footer">
  AutoGuard v1.0.0 &mdash; Aggiornamento ogni 2s
</div>

<script>
const zoneNames  = ["--", "CRITICA", "MEDIA", "LONTANA"];
const zoneColors = ["#334155", "#ef4444", "#f97316", "#eab308"];
const zonePct    = [0, 100, 60, 30];

async function fetchStatus() {
  try {
    const r = await fetch('/api/status');
    const d = await r.json();
    updateUI(d);
  } catch(e) {
    console.error('Errore fetch:', e);
  }
}

function updateUI(d) {
  // Stato
  const badge = document.getElementById('stateBadge');
  badge.textContent = d.state;
  badge.className = 'state-badge state-' + d.state;

  // Elapsed
  const sec = Math.floor(d.elapsed_ms / 1000);
  document.getElementById('stateElapsed').textContent =
    sec + 's nello stato corrente';

  // Arming countdown
  const armInfo = document.getElementById('armingInfo');
  if (d.state === 'ARMING' && d.arming_ms > 0) {
    armInfo.style.display = 'block';
    document.getElementById('armingCountdown').textContent =
      Math.ceil(d.arming_ms / 1000);
  } else {
    armInfo.style.display = 'none';
  }

  // Radar
  const det = d.radar.detected;
  const dot = document.getElementById('detDot');
  dot.className = 'dot ' + (det ? 'dot-on' : 'dot-off');
  document.getElementById('detLabel').textContent =
    det ? 'Presenza rilevata!' : 'Nessuna presenza';
  document.getElementById('detLabel').style.color =
    det ? '#22c55e' : '#94a3b8';

  document.getElementById('radarDist').textContent =
    det ? d.radar.distance : '--';

  const zone = d.radar.zone;
  document.getElementById('zoneFill').style.width  = zonePct[zone] + '%';
  document.getElementById('zoneFill').style.background = zoneColors[zone];
  document.getElementById('zoneLabel').textContent =
    'Zona: ' + zoneNames[zone];

  // Info
  document.getElementById('infoIP').textContent    = d.wifi.ip;
  document.getElementById('infoRSSI').textContent  = d.wifi.rssi + ' dBm';
  document.getElementById('infoUptime').textContent =
    formatUptime(d.uptime_s);
  document.getElementById('infoHeap').textContent  =
    Math.round(d.free_heap / 1024) + ' KB';
  document.getElementById('infoFW').textContent    = d.fw_version;
}

function formatUptime(s) {
  const h = Math.floor(s / 3600);
  const m = Math.floor((s % 3600) / 60);
  const sec = s % 60;
  return h + 'h ' + m + 'm ' + sec + 's';
}

async function sendCmd(cmd) {
  const fb = document.getElementById('cmdFeedback');
  try {
    fb.textContent = 'Invio comando: ' + cmd.toUpperCase() + '...';
    fb.style.color = '#94a3b8';
    const r = await fetch('/api/' + cmd, { method: 'POST' });
    const d = await r.json();
    fb.textContent = 'Comando ' + cmd.toUpperCase() + ' inviato!';
    fb.style.color = '#34d399';
    fetchStatus();
  } catch(e) {
    fb.textContent = 'Errore invio comando!';
    fb.style.color = '#f87171';
  }
}

// Aggiorna ogni 2 secondi
setInterval(fetchStatus, 2000);
fetchStatus();
</script>
</body>
</html>
)rawhtml";
}

// ============================================================
// isConnected() / getIP()
// ============================================================
bool AutoGuardWeb::isConnected() {
    return _wifiConnected && WiFi.status() == WL_CONNECTED;
}

String AutoGuardWeb::getIP() {
    return WiFi.localIP().toString();
}
