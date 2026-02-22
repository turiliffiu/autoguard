// ============================================================
// AutoGuard - Web Server + Dashboard
// ============================================================
#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "config.h"
#include "alarm_logic.h"
#include "sensor_ld2420.h"

class AutoGuardWeb {
public:
    AutoGuardWeb(AlarmLogic& alarmSys, SensorLD2420& radar);

    // Inizializza WiFi e web server
    bool begin();

    // Da chiamare nel loop
    void update();

    // true se WiFi connesso
    bool isConnected();

    // IP corrente
    String getIP();

private:
    AsyncWebServer  _server;
    AlarmLogic&     _alarmSys;
    SensorLD2420&   _radar;
    bool            _wifiConnected;

    // Setup WiFi
    bool _connectWiFi();

    // Setup routes
    void _setupRoutes();

    // Genera JSON stato sistema
    String _buildStatusJson();

    // Genera HTML dashboard
    String _buildDashboardHtml();
};

#endif // WEB_SERVER_H
