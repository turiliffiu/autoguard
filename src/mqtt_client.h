// ============================================================
// AutoGuard - MQTT Client
// ============================================================
#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "alarm_logic.h"
#include "sensor_ld2420.h"

class AutoGuardMQTT {
public:
    AutoGuardMQTT(AlarmLogic& alarmSys, SensorLD2420& radar);

    bool begin();
    void update();
    void publishStatus();
    bool isConnected();

private:
    WiFiClient      _wifiClient;
    PubSubClient    _mqtt;
    AlarmLogic&     _alarmSys;
    SensorLD2420&   _radar;

    uint32_t _lastPublish;
    uint32_t _lastReconnect;

    bool _connect();

    // Discovery
    void _publishDiscovery();
    void _publishDiscoverySensor(const char* id, const char* name,
                                  const char* stateTopic, const char* valueTemplate,
                                  const char* unit, const char* devClass);
    void _publishDiscoveryBinarySensor(const char* id, const char* name,
                                        const char* stateTopic, const char* valueTemplate,
                                        const char* devClass, const char* payloadOn,
                                        const char* payloadOff);
    void _publishDiscoveryButton(const char* id, const char* name,
                                  const char* cmdTopic, const char* payload);

    // Helpers
    static void _onMessage(char* topic, byte* payload, unsigned int len);
    static AutoGuardMQTT* _instance;

    void _publishRadar();
    void _publishAlert(const AlarmEvent& ev);
    String _buildStatusJson();
    String _buildRadarJson();
};

#endif // MQTT_CLIENT_H
