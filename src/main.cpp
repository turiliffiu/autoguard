// ============================================================
// AutoGuard - Antifurto Auto ESP32-C6 + HLK-LD2420
// Main entry point
// ============================================================
#include <Arduino.h>
#include "config.h"
#include "sensor_ld2420.h"
#include "alarm_logic.h"
#include "web_server.h"
#include "mqtt_client.h"

// ============================================================
// Istanze globali
// ============================================================
SensorLD2420  radar;
AlarmLogic    alarmSys;
AutoGuardWeb  webServer(alarmSys, radar);
AutoGuardMQTT mqttClient(alarmSys, radar);

// ============================================================
// LED helpers
// ============================================================
static uint32_t lastLedToggle = 0;
static bool     ledState      = false;

void updateLED() {
    AlarmState state = alarmSys.getState();
    uint32_t now     = millis();

    // ALARM → LED fisso acceso
    if (state == STATE_ALARM) {
        digitalWrite(LED_STATUS_PIN, HIGH);
        ledState = true;
        return;
    }

    // Frequenza blink per ogni stato
    uint32_t interval = 2000; // DISARMED
    switch (state) {
        case STATE_ARMING:   interval = 500;  break;
        case STATE_ARMED:    interval = 200;  break;
        case STATE_ALERT:    interval = 50;   break;
        case STATE_COOLDOWN: interval = 1000; break;
        default: break;
    }

    if (now - lastLedToggle >= interval) {
        ledState = !ledState;
        digitalWrite(LED_STATUS_PIN, ledState);
        lastLedToggle = now;
    }
}

// ============================================================
// Comandi seriali (debug)
// ============================================================
void handleSerial() {
    if (!Serial.available()) return;
    char c = Serial.read();
    switch (c) {
        case 'a':
            alarmSys.arm();
            Serial.println("[CMD] arm");
            mqttClient.publishStatus();
            break;
        case 'd':
            alarmSys.disarm();
            Serial.println("[CMD] disarm");
            mqttClient.publishStatus();
            break;
        case 'r':
            alarmSys.reset();
            Serial.println("[CMD] reset");
            mqttClient.publishStatus();
            break;
        case 's':
            Serial.printf("[STATUS] Stato: %s | Radar: %dcm | WiFi: %s | MQTT: %s\n",
                alarmSys.getStateName(),
                radar.getData().filtered_dist,
                webServer.isConnected() ? webServer.getIP().c_str() : "NON CONNESSO",
                mqttClient.isConnected() ? "OK" : "OFFLINE");
            break;
        default: break;
    }
}

// ============================================================
// setup()
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("============================================");
    Serial.printf("  AutoGuard v%s\n", FIRMWARE_VERSION);
    Serial.println("  Antifurto Auto ESP32-C6 + HLK-LD2420");
    Serial.println("============================================");

    // LED
    pinMode(LED_STATUS_PIN, OUTPUT);
    digitalWrite(LED_STATUS_PIN, LOW);

    // Radar
    Serial.println("[SETUP] Inizializzazione radar...");
    if (radar.begin()) {
        Serial.println("[SETUP] Radar OK");
    } else {
        Serial.println("[SETUP] WARN: Radar non risponde (cavo?)");
    }

    // Web Server (WiFi + HTTP)
    Serial.println("[SETUP] Connessione WiFi + Web Server...");
    if (webServer.begin()) {
        Serial.printf("[SETUP] Web server: http://%s\n",
            webServer.getIP().c_str());

        // MQTT (solo se WiFi disponibile)
        Serial.println("[SETUP] Connessione MQTT...");
        if (mqttClient.begin()) {
            Serial.println("[SETUP] MQTT OK");
        } else {
            Serial.println("[SETUP] WARN: MQTT non disponibile");
        }
    } else {
        Serial.println("[SETUP] WARN: WiFi non disponibile");
    }

    Serial.println("[SETUP] Completato!");
    Serial.println("Comandi: a=arm  d=disarm  r=reset  s=status");
    Serial.println("============================================");
}

// ============================================================
// loop()
// ============================================================
void loop() {
    // 1. Aggiorna radar
    radar.update();

    // 2. Aggiorna logica allarme
    RadarData data = radar.getData();
    alarmSys.update(data);

    // 3. Aggiorna LED
    updateLED();

    // 4. Aggiorna web (riconnessione WiFi)
    webServer.update();

    // 5. Aggiorna MQTT (publish + subscribe)
    mqttClient.update();

    // 6. Comandi seriali debug
    handleSerial();
}
