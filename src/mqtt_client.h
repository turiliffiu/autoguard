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

    // Inizializza e connette
    bool begin();

    // Da chiamare nel loop
    void update();

    // Pubblica stato corrente manualmente
    void publishStatus();

    bool isConnected();

private:
    WiFiClient      _wifiClient;
    PubSubClient    _mqtt;
    AlarmLogic&     _alarmSys;
    SensorLD2420&   _radar;

    uint32_t _lastPublish;      // Timer publish periodico
    uint32_t _lastReconnect;    // Timer reconnect

    // Connessione al broker
    bool _connect();

    // Callback messaggi in arrivo
    static void _onMessage(char* topic, byte* payload, unsigned int len);
    static AutoGuardMQTT* _instance; // per callback statica

    // Pubblica JSON radar
    void _publishRadar();

    // Pubblica JSON alert/evento
    void _publishAlert(const AlarmEvent& ev);

    // Costruisce JSON stato
    String _buildStatusJson();

    // Costruisce JSON radar
    String _buildRadarJson();
};

#endif // MQTT_CLIENT_H
