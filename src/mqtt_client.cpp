// ============================================================
// AutoGuard - MQTT Client - Implementazione
// ============================================================
#include "mqtt_client.h"

// Istanza statica per callback
AutoGuardMQTT* AutoGuardMQTT::_instance = nullptr;

// ============================================================
// Costruttore
// ============================================================
AutoGuardMQTT::AutoGuardMQTT(AlarmLogic& alarmSys, SensorLD2420& radar) :
    _mqtt(_wifiClient),
    _alarmSys(alarmSys),
    _radar(radar),
    _lastPublish(0),
    _lastReconnect(0)
{
    _instance = this;
}

// ============================================================
// begin()
// ============================================================
bool AutoGuardMQTT::begin() {
    _mqtt.setServer(MQTT_BROKER, MQTT_PORT);
    _mqtt.setCallback(_onMessage);
    _mqtt.setKeepAlive(30);
    _mqtt.setBufferSize(512);

    Serial.printf("[MQTT] Broker: %s:%d\n", MQTT_BROKER, MQTT_PORT);
    return _connect();
}

// ============================================================
// _connect() - Connessione con LWT
// ============================================================
bool AutoGuardMQTT::_connect() {
    if (!WiFi.isConnected()) {
        Serial.println("[MQTT] WiFi non disponibile");
        return false;
    }

    Serial.printf("[MQTT] Connessione a %s...\n", MQTT_BROKER);

    // LWT = Last Will Testament: pubblicato dal broker se ESP32 si disconnette
    String lwt = "{\"state\":\"OFFLINE\",\"device\":\"" MQTT_CLIENT_ID "\"}";

    bool ok = _mqtt.connect(
        MQTT_CLIENT_ID,
        MQTT_USER,
        MQTT_PASS,
        MQTT_TOPIC_STATUS,   // LWT topic
        1,                   // LWT QoS
        true,                // LWT retain
        lwt.c_str()
    );

    if (!ok) {
        Serial.printf("[MQTT] ERRORE connessione, rc=%d\n", _mqtt.state());
        return false;
    }

    Serial.println("[MQTT] Connesso!");

    // Subscribe ai comandi
    _mqtt.subscribe(MQTT_TOPIC_CMD);
    Serial.printf("[MQTT] Subscribed: %s\n", MQTT_TOPIC_CMD);

    // Publish stato online
    publishStatus();

    return true;
}

// ============================================================
// update() - Loop MQTT
// ============================================================
void AutoGuardMQTT::update() {
    uint32_t now = millis();

    // Riconnessione se necessario
    if (!_mqtt.connected()) {
        if (now - _lastReconnect > MQTT_RECONNECT_MS) {
            _lastReconnect = now;
            Serial.println("[MQTT] Riconnessione...");
            _connect();
        }
        return;
    }

    // Loop PubSubClient
    _mqtt.loop();

    // Pubblica evento se ce n'è uno nuovo
    if (_alarmSys.hasNewEvent()) {
        AlarmEvent ev = _alarmSys.getLastEvent();
        publishStatus();
        _publishAlert(ev);
    }

    // Publish periodico radar + status
    if (now - _lastPublish > MQTT_PUBLISH_MS) {
        _lastPublish = now;
        _publishRadar();
    }
}

// ============================================================
// _onMessage() - Callback comandi in arrivo
// ============================================================
void AutoGuardMQTT::_onMessage(char* topic, byte* payload, unsigned int len) {
    if (!_instance) return;

    // Converti payload in stringa
    char buf[64] = {0};
    size_t copyLen = (len < sizeof(buf)-1) ? len : sizeof(buf)-1;
    memcpy(buf, payload, copyLen);
    String msg = String(buf);
    msg.trim();
    msg.toLowerCase();

    Serial.printf("[MQTT] Ricevuto [%s]: %s\n", topic, msg.c_str());

    // Gestione comandi
    if (msg == "arm") {
        _instance->_alarmSys.arm();
        _instance->publishStatus();
    } else if (msg == "disarm") {
        _instance->_alarmSys.disarm();
        _instance->publishStatus();
    } else if (msg == "reset") {
        _instance->_alarmSys.reset();
        _instance->publishStatus();
    } else if (msg == "status") {
        _instance->publishStatus();
    } else {
        Serial.printf("[MQTT] Comando sconosciuto: %s\n", msg.c_str());
    }
}

// ============================================================
// publishStatus() - Pubblica stato sistema
// ============================================================
void AutoGuardMQTT::publishStatus() {
    if (!_mqtt.connected()) return;

    String json = _buildStatusJson();
    bool ok = _mqtt.publish(MQTT_TOPIC_STATUS, json.c_str(), true); // retain=true
    Serial.printf("[MQTT] Status publish %s\n", ok ? "OK" : "FAIL");
}

// ============================================================
// _publishRadar() - Pubblica dati radar
// ============================================================
void AutoGuardMQTT::_publishRadar() {
    if (!_mqtt.connected()) return;

    String json = _buildRadarJson();
    _mqtt.publish(MQTT_TOPIC_SENSOR, json.c_str(), false);
}

// ============================================================
// _publishAlert() - Pubblica evento allarme
// ============================================================
void AutoGuardMQTT::_publishAlert(const AlarmEvent& ev) {
    if (!_mqtt.connected()) return;

    JsonDocument doc;
    doc["event"]       = "state_change";
    doc["state"]       = _alarmSys.getStateName();
    doc["prev_state"]  = ev.prevState;
    doc["zone"]        = (int)ev.zone;
    doc["distance"]    = ev.distance_cm;
    doc["timestamp"]   = ev.timestamp;
    doc["uptime_s"]    = millis() / 1000;

    String json;
    serializeJson(doc, json);

    bool ok = _mqtt.publish(MQTT_TOPIC_ALERT, json.c_str(), false);
    Serial.printf("[MQTT] Alert publish %s: %s\n", ok ? "OK" : "FAIL", json.c_str());
}

// ============================================================
// _buildStatusJson()
// ============================================================
String AutoGuardMQTT::_buildStatusJson() {
    JsonDocument doc;
    RadarData radarData = _radar.getData();

    doc["state"]      = _alarmSys.getStateName();
    doc["state_id"]   = (int)_alarmSys.getState();
    doc["device"]     = MQTT_CLIENT_ID;
    doc["fw"]         = FIRMWARE_VERSION;
    doc["uptime_s"]   = millis() / 1000;
    doc["free_heap"]  = ESP.getFreeHeap();
    doc["ip"]         = WiFi.localIP().toString();
    doc["rssi"]       = WiFi.RSSI();

    JsonObject radar = doc["radar"].to<JsonObject>();
    radar["detected"]  = radarData.detected;
    radar["distance"]  = radarData.filtered_dist;
    radar["zone"]      = (int)radarData.zone;

    String out;
    serializeJson(doc, out);
    return out;
}

// ============================================================
// _buildRadarJson()
// ============================================================
String AutoGuardMQTT::_buildRadarJson() {
    JsonDocument doc;
    RadarData d = _radar.getData();

    doc["detected"]   = d.detected;
    doc["distance"]   = d.filtered_dist;
    doc["raw_dist"]   = d.distance_cm;
    doc["zone"]       = (int)d.zone;
    doc["timestamp"]  = d.timestamp;

    String out;
    serializeJson(doc, out);
    return out;
}

// ============================================================
// isConnected()
// ============================================================
bool AutoGuardMQTT::isConnected() {
    return _mqtt.connected();
}
