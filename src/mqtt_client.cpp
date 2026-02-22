// ============================================================
// AutoGuard - MQTT Client - Implementazione
// ============================================================
#include "mqtt_client.h"

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
    _mqtt.setBufferSize(1024); // Discovery JSON è grande
    Serial.printf("[MQTT] Broker: %s:%d\n", MQTT_BROKER, MQTT_PORT);
    return _connect();
}

// ============================================================
// _connect()
// ============================================================
bool AutoGuardMQTT::_connect() {
    if (!WiFi.isConnected()) {
        Serial.println("[MQTT] WiFi non disponibile");
        return false;
    }

    Serial.printf("[MQTT] Connessione a %s...\n", MQTT_BROKER);

    String lwt = "{\"state\":\"OFFLINE\",\"device\":\"" MQTT_CLIENT_ID "\"}";

    bool ok = _mqtt.connect(
        MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS,
        MQTT_TOPIC_STATUS, 1, true, lwt.c_str()
    );

    if (!ok) {
        Serial.printf("[MQTT] ERRORE rc=%d\n", _mqtt.state());
        return false;
    }

    Serial.println("[MQTT] Connesso!");
    _mqtt.subscribe(MQTT_TOPIC_CMD);
    Serial.printf("[MQTT] Subscribed: %s\n", MQTT_TOPIC_CMD);

    // Pubblica Discovery per Home Assistant
    _publishDiscovery();

    // Pubblica stato iniziale
    publishStatus();
    return true;
}

// ============================================================
// update()
// ============================================================
void AutoGuardMQTT::update() {
    uint32_t now = millis();

    if (!_mqtt.connected()) {
        if (now - _lastReconnect > MQTT_RECONNECT_MS) {
            _lastReconnect = now;
            Serial.println("[MQTT] Riconnessione...");
            _connect();
        }
        return;
    }

    _mqtt.loop();

    if (_alarmSys.hasNewEvent()) {
        AlarmEvent ev = _alarmSys.getLastEvent();
        _alarmSys.clearNewEvent();
        publishStatus();
        _publishAlert(ev);
    }

    if (now - _lastPublish > MQTT_PUBLISH_MS) {
        _lastPublish = now;
        _publishRadar();
    }
}

// ============================================================
// _onMessage() - Comandi da HA o MQTT
// ============================================================
void AutoGuardMQTT::_onMessage(char* topic, byte* payload, unsigned int len) {
    if (!_instance) return;

    char buf[64] = {0};
    size_t n = (len < sizeof(buf)-1) ? len : sizeof(buf)-1;
    memcpy(buf, payload, n);
    String msg = String(buf);
    msg.trim();
    msg.toLowerCase();

    Serial.printf("[MQTT] Ricevuto [%s]: %s\n", topic, msg.c_str());

    if      (msg == "arm")    { _instance->_alarmSys.arm();    _instance->publishStatus(); }
    else if (msg == "disarm") { _instance->_alarmSys.disarm(); _instance->publishStatus(); }
    else if (msg == "reset")  { _instance->_alarmSys.reset();  _instance->publishStatus(); }
    else if (msg == "status") { _instance->publishStatus(); }
    else { Serial.printf("[MQTT] Comando sconosciuto: %s\n", msg.c_str()); }
}

// ============================================================
// _publishDiscovery() - Registra entità in Home Assistant
// ============================================================
void AutoGuardMQTT::_publishDiscovery() {
    Serial.println("[MQTT] Pubblicazione MQTT Discovery...");

    // 1. Sensor: Stato sistema
    _publishDiscoverySensor(
        "stato", "Stato",
        MQTT_TOPIC_STATUS, "{{ value_json.state }}",
        nullptr, nullptr
    );

    // 2. Sensor: Distanza radar
    _publishDiscoverySensor(
        "distanza", "Distanza Radar",
        MQTT_TOPIC_SENSOR, "{{ value_json.distance }}",
        "cm", "distance"
    );

    // 3. Binary sensor: Presenza
    _publishDiscoveryBinarySensor(
        "presenza", "Presenza Rilevata",
        MQTT_TOPIC_SENSOR, "{{ value_json.detected }}",
        "motion", "True", "False"
    );

    // 4. Binary sensor: Allarme attivo
    _publishDiscoveryBinarySensor(
        "allarme", "Allarme",
        MQTT_TOPIC_STATUS, "{{ value_json.state }}",
        "safety", "ALARM", "DISARMED"
    );

    // 5. Button: Arm
    _publishDiscoveryButton("arm_btn", "Arma", MQTT_TOPIC_CMD, "arm");

    // 6. Button: Disarm
    _publishDiscoveryButton("disarm_btn", "Disarma", MQTT_TOPIC_CMD, "disarm");

    // 7. Button: Reset
    _publishDiscoveryButton("reset_btn", "Reset Allarme", MQTT_TOPIC_CMD, "reset");

    Serial.println("[MQTT] Discovery completata!");
}

// ============================================================
// _publishDiscoverySensor()
// ============================================================
void AutoGuardMQTT::_publishDiscoverySensor(
    const char* id, const char* name,
    const char* stateTopic, const char* valueTemplate,
    const char* unit, const char* devClass)
{
    // Topic: homeassistant/sensor/autoguard_001_stato/config
    char topic[128];
    snprintf(topic, sizeof(topic), "%s/sensor/%s_%s/config",
        MQTT_DISCOVERY_PREFIX, MQTT_DEVICE_ID, id);

    JsonDocument doc;
    doc["name"]           = name;
    doc["unique_id"]      = String(MQTT_DEVICE_ID) + "_" + id;
    doc["state_topic"]    = stateTopic;
    doc["value_template"] = valueTemplate;
    if (unit)    doc["unit_of_measurement"] = unit;
    if (devClass) doc["device_class"]       = devClass;

    // Device info (raggruppa tutte le entità sotto un dispositivo)
    JsonObject dev = doc["device"].to<JsonObject>();
    dev["identifiers"][0]  = MQTT_DEVICE_ID;
    dev["name"]            = MQTT_DEVICE_NAME;
    dev["model"]           = MQTT_DEVICE_MODEL;
    dev["manufacturer"]    = MQTT_DEVICE_MANUFACTURER;
    dev["sw_version"]      = FIRMWARE_VERSION;

    String json;
    serializeJson(doc, json);
    bool ok = _mqtt.publish(topic, json.c_str(), true); // retain=true
    Serial.printf("[MQTT] Discovery sensor '%s': %s\n", id, ok ? "OK" : "FAIL");
}

// ============================================================
// _publishDiscoveryBinarySensor()
// ============================================================
void AutoGuardMQTT::_publishDiscoveryBinarySensor(
    const char* id, const char* name,
    const char* stateTopic, const char* valueTemplate,
    const char* devClass, const char* payloadOn, const char* payloadOff)
{
    char topic[128];
    snprintf(topic, sizeof(topic), "%s/binary_sensor/%s_%s/config",
        MQTT_DISCOVERY_PREFIX, MQTT_DEVICE_ID, id);

    JsonDocument doc;
    doc["name"]           = name;
    doc["unique_id"]      = String(MQTT_DEVICE_ID) + "_" + id;
    doc["state_topic"]    = stateTopic;
    doc["value_template"] = valueTemplate;
    doc["payload_on"]     = payloadOn;
    doc["payload_off"]    = payloadOff;
    if (devClass) doc["device_class"] = devClass;

    JsonObject dev = doc["device"].to<JsonObject>();
    dev["identifiers"][0]  = MQTT_DEVICE_ID;
    dev["name"]            = MQTT_DEVICE_NAME;
    dev["model"]           = MQTT_DEVICE_MODEL;
    dev["manufacturer"]    = MQTT_DEVICE_MANUFACTURER;
    dev["sw_version"]      = FIRMWARE_VERSION;

    String json;
    serializeJson(doc, json);
    bool ok = _mqtt.publish(topic, json.c_str(), true);
    Serial.printf("[MQTT] Discovery binary_sensor '%s': %s\n", id, ok ? "OK" : "FAIL");
}

// ============================================================
// _publishDiscoveryButton()
// ============================================================
void AutoGuardMQTT::_publishDiscoveryButton(
    const char* id, const char* name,
    const char* cmdTopic, const char* payload)
{
    char topic[128];
    snprintf(topic, sizeof(topic), "%s/button/%s_%s/config",
        MQTT_DISCOVERY_PREFIX, MQTT_DEVICE_ID, id);

    JsonDocument doc;
    doc["name"]             = name;
    doc["unique_id"]        = String(MQTT_DEVICE_ID) + "_" + id;
    doc["command_topic"]    = cmdTopic;
    doc["payload_press"]    = payload;

    JsonObject dev = doc["device"].to<JsonObject>();
    dev["identifiers"][0]  = MQTT_DEVICE_ID;
    dev["name"]            = MQTT_DEVICE_NAME;
    dev["model"]           = MQTT_DEVICE_MODEL;
    dev["manufacturer"]    = MQTT_DEVICE_MANUFACTURER;
    dev["sw_version"]      = FIRMWARE_VERSION;

    String json;
    serializeJson(doc, json);
    bool ok = _mqtt.publish(topic, json.c_str(), true);
    Serial.printf("[MQTT] Discovery button '%s': %s\n", id, ok ? "OK" : "FAIL");
}

// ============================================================
// publishStatus()
// ============================================================
void AutoGuardMQTT::publishStatus() {
    if (!_mqtt.connected()) return;
    String json = _buildStatusJson();
    bool ok = _mqtt.publish(MQTT_TOPIC_STATUS, json.c_str(), true);
    Serial.printf("[MQTT] Status publish %s\n", ok ? "OK" : "FAIL");
}

// ============================================================
// _publishRadar()
// ============================================================
void AutoGuardMQTT::_publishRadar() {
    if (!_mqtt.connected()) return;
    String json = _buildRadarJson();
    _mqtt.publish(MQTT_TOPIC_SENSOR, json.c_str(), false);
}

// ============================================================
// _publishAlert()
// ============================================================
void AutoGuardMQTT::_publishAlert(const AlarmEvent& ev) {
    if (!_mqtt.connected()) return;

    JsonDocument doc;
    doc["event"]      = "state_change";
    doc["state"]      = _alarmSys.getStateName();
    doc["prev_state"] = ev.prevState;
    doc["zone"]       = (int)ev.zone;
    doc["distance"]   = ev.distance_cm;
    doc["timestamp"]  = ev.timestamp;
    doc["uptime_s"]   = millis() / 1000;

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
    RadarData d = _radar.getData();

    doc["state"]     = _alarmSys.getStateName();
    doc["state_id"]  = (int)_alarmSys.getState();
    doc["device"]    = MQTT_CLIENT_ID;
    doc["fw"]        = FIRMWARE_VERSION;
    doc["uptime_s"]  = millis() / 1000;
    doc["free_heap"] = ESP.getFreeHeap();
    doc["ip"]        = WiFi.localIP().toString();
    doc["rssi"]      = WiFi.RSSI();

    JsonObject radar = doc["radar"].to<JsonObject>();
    radar["detected"] = d.detected;
    radar["distance"] = d.filtered_dist;
    radar["zone"]     = (int)d.zone;

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

    doc["detected"]  = d.detected;
    doc["distance"]  = d.filtered_dist;
    doc["raw_dist"]  = d.distance_cm;
    doc["zone"]      = (int)d.zone;
    doc["timestamp"] = d.timestamp;

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
