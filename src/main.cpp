// ============================================================
// AutoGuard - Antifurto Auto ESP32-C6 + HLK-LD2420
// Versione: 1.0.0
// Autore: Salvo
// ============================================================
#include <Arduino.h>
#include "config.h"
#include "version.h"
#include "sensor_ld2420.h"

// Istanza sensore
SensorLD2420 radar;

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
        // Blink veloce per segnalare errore
        while (true) {
            digitalWrite(LED_STATUS_PIN, HIGH);
            delay(100);
            digitalWrite(LED_STATUS_PIN, LOW);
            delay(100);
        }
    }

    Serial.println("[BOOT] Sistema pronto!");
    Serial.println("[BOOT] In attesa rilevamenti radar...");
}

void loop() {
    // Aggiorna letture radar
    radar.update();

    // Leggi dati
    RadarData data = radar.getData();

    // Stampa stato ogni secondo
    static uint32_t lastPrint = 0;
    if (millis() - lastPrint > 1000) {
        lastPrint = millis();

        if (data.detected) {
            const char* zoneNames[] = {"NONE", "CRITICA", "MEDIA", "LONTANA"};
            Serial.printf("[LOOP] RILEVATO! Dist:%dcm Zona:%s Consec:%d\n",
                data.filtered_dist,
                zoneNames[data.zone],
                radar.getConsecutiveDetections());

            // LED acceso se rilevato
            digitalWrite(LED_STATUS_PIN, HIGH);
        } else {
            Serial.println("[LOOP] Nessuna presenza rilevata");
            // LED spento
            digitalWrite(LED_STATUS_PIN, LOW);
        }
    }

    delay(10);
}
