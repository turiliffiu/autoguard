// ============================================================
// AutoGuard - Antifurto Auto ESP32-C6 + HLK-LD2420
// ============================================================
#include <Arduino.h>
#include "config.h"
#include "sensor_ld2420.h"
#include "alarm_logic.h"
#include "web_server.h"
#include "mqtt_client.h"

// ============================================================
// Oggetti semplici (sicuri come globali)
// ============================================================
SensorLD2420  radar;
AlarmLogic    alarmSys;

// ============================================================
// Oggetti complessi → puntatori, init in setup()
// ============================================================
AutoGuardWeb*  webServer  = nullptr;
AutoGuardMQTT* mqttClient = nullptr;

// ============================================================
// LED helpers
// ============================================================
#ifdef LED_ACTIVE_LOW
  #define LED_ON  LOW
  #define LED_OFF HIGH
#else
  #define LED_ON  HIGH
  #define LED_OFF LOW
#endif

static uint32_t lastLedToggle = 0;
static bool     ledState      = false;

void updateLED() {
    AlarmState state = alarmSys.getState();
    uint32_t now     = millis();

    if (state == STATE_ALARM) {
        digitalWrite(LED_STATUS_PIN, LED_ON);
        ledState = true;
        return;
    }

    uint32_t interval = 2000;
    switch (state) {
        case STATE_ARMING:   interval = 500;  break;
        case STATE_ARMED:    interval = 200;  break;
        case STATE_ALERT:    interval = 50;   break;
        case STATE_COOLDOWN: interval = 1000; break;
        default: break;
    }

    if (now - lastLedToggle >= interval) {
        ledState = !ledState;
        digitalWrite(LED_STATUS_PIN, ledState ? LED_ON : LED_OFF);
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
            if (mqttClient) mqttClient->publishStatus();
            break;
        case 'd':
            alarmSys.disarm();
            Serial.println("[CMD] disarm");
            if (mqttClient) mqttClient->publishStatus();
            break;
        case 'r':
            alarmSys.reset();
            Serial.println("[CMD] reset");
            if (mqttClient) mqttClient->publishStatus();
            break;
        case 's':
            Serial.printf("[STATUS] Stato: %s | Radar: %dcm | WiFi: %s | MQTT: %s\n",
                alarmSys.getStateName(),
                radar.getData().filtered_dist,
                (webServer && webServer->isConnected()) ? webServer->getIP().c_str() : "NO",
                (mqttClient && mqttClient->isConnected()) ? "OK" : "OFFLINE");
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
    digitalWrite(LED_STATUS_PIN, LED_OFF);

    // Radar
    Serial.println("[SETUP] Inizializzazione radar...");
    if (radar.begin()) {
        Serial.println("[SETUP] Radar OK");
    } else {
        Serial.println("[SETUP] WARN: Radar non risponde");
    }

    // Web Server
    Serial.println("[SETUP] Avvio Web Server...");
    webServer = new AutoGuardWeb(alarmSys, radar);
    if (webServer->begin()) {
        Serial.printf("[SETUP] Web server: http://%s\n",
            webServer->getIP().c_str());

        // MQTT (solo se WiFi ok)
        Serial.println("[SETUP] Avvio MQTT...");
        mqttClient = new AutoGuardMQTT(alarmSys, radar);
        if (mqttClient->begin()) {
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
    radar.update();

    RadarData data = radar.getData();
    alarmSys.update(data);

    updateLED();

    if (webServer)  webServer->update();
    if (mqttClient) mqttClient->update();

    handleSerial();
}
