// ============================================================
// AutoGuard - Antifurto Auto ESP32-C6 + HLK-LD2420
// Versione: 1.0.0
// Autore: Salvo
// ============================================================
#include <Arduino.h>
#include "config.h"
#include "version.h"

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("========================================");
    Serial.println("  AutoGuard v" FW_VERSION);
    Serial.println("  Antifurto Auto ESP32-C6 + LD2420");
    Serial.println("========================================");
    Serial.printf("  Build: %s %s\n", BUILD_DATE, BUILD_TIME);
    Serial.println("========================================");

    pinMode(LED_STATUS_PIN, OUTPUT);
    digitalWrite(LED_STATUS_PIN, LOW);

    Serial.println("[BOOT] Setup completato - sviluppo in corso...");
}

void loop() {
    // Blink LED per segnalare che il firmware gira
    digitalWrite(LED_STATUS_PIN, HIGH);
    delay(500);
    digitalWrite(LED_STATUS_PIN, LOW);
    delay(500);
}
