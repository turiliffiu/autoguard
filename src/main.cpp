// ============================================================
// AutoGuard - Antifurto Auto ESP32-C6 + HLK-LD2420
// Versione: 1.0.0
// Autore: Salvo
// ============================================================
#include <Arduino.h>
#include "config.h"
#include "version.h"
#include "sensor_ld2420.h"
#include "alarm_logic.h"

// Istanze
SensorLD2420 radar;
AlarmLogic   alarmSys;

// LED blink non-bloccante
uint32_t lastBlink = 0;
bool     ledState  = false;

void updateLED() {
    AlarmState state = alarmSys.getState();
    uint32_t blinkMs = 0;

    switch (state) {
        case STATE_DISARMED:  blinkMs = 2000; break;  // lento
        case STATE_ARMING:    blinkMs = 500;  break;  // medio
        case STATE_ARMED:     blinkMs = 200;  break;  // veloce
        case STATE_ALERT:     blinkMs = 50;   break;  // rapidissimo
        case STATE_ALARM:
            // Fisso acceso - gestito da alarm_logic
            return;
        case STATE_COOLDOWN:  blinkMs = 1000; break;  // medio-lento
    }

    if (millis() - lastBlink > blinkMs) {
        lastBlink = millis();
        ledState = !ledState;
        digitalWrite(LED_STATUS_PIN, ledState);
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("========================================");
    Serial.println("  AutoGuard v" FW_VERSION);
    Serial.println("  Antifurto Auto ESP32-C6 + LD2420");
    Serial.println("========================================");
    Serial.printf("  Build: %s %s\n", BUILD_DATE, BUILD_TIME);
    Serial.println("========================================");

    // LED status
    pinMode(LED_STATUS_PIN, OUTPUT);
    digitalWrite(LED_STATUS_PIN, LOW);

    // Inizializza sensore radar
    if (!radar.begin()) {
        Serial.println("[BOOT] ERRORE: radar non inizializzato!");
        while (true) {
            digitalWrite(LED_STATUS_PIN, HIGH);
            delay(100);
            digitalWrite(LED_STATUS_PIN, LOW);
            delay(100);
        }
    }

    // Inizializza alarm logic
    alarmSys.begin();

    Serial.println("[BOOT] Sistema pronto!");
    Serial.println("[BOOT] Comandi Serial: 'a'=arm  'd'=disarm  'r'=reset  's'=status");
}

void loop() {
    // 1. Aggiorna radar
    radar.update();
    RadarData data = radar.getData();

    // 2. Aggiorna state machine
    alarmSys.update(data);

    // 3. Aggiorna LED
    updateLED();

    // 4. Comandi da Serial (per test senza web/MQTT)
    if (Serial.available()) {
        char cmd = Serial.read();
        switch (cmd) {
            case 'a': case 'A':
                alarmSys.arm();
                break;
            case 'd': case 'D':
                alarmSys.disarm();
                break;
            case 'r': case 'R':
                alarmSys.reset();
                break;
            case 's': case 'S':
                Serial.printf("[STATUS] Stato:%s Radar:%s Dist:%dcm\n",
                    alarmSys.getStateName(),
                    data.detected ? "RILEVATO" : "libero",
                    data.filtered_dist);
                break;
        }
    }

    delay(10);
}
