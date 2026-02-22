// ============================================================
// AutoGuard - Configurazione Hardware e Software
// ============================================================
#ifndef CONFIG_H
#define CONFIG_H

// ------------------------------------------------------------
// VERSIONE FIRMWARE
// ------------------------------------------------------------
#define FIRMWARE_VERSION    "1.0.0"
#define PROJECT_NAME        "AutoGuard"
#define BUILD_DATE          __DATE__
#define BUILD_TIME          __TIME__

// ------------------------------------------------------------
// PIN ESP32-C6 → HLK-LD2420 (UART1)
// ------------------------------------------------------------
#define RADAR_RX_PIN        4       // GPIO4 ← TX sensore (OT1/J2P3)
#define RADAR_TX_PIN        5       // GPIO5 → RX sensore (OT2/J2P4)
#define RADAR_BAUD          115200

// ------------------------------------------------------------
// PIN LED DI STATO
// ------------------------------------------------------------
#define LED_STATUS_PIN      8       // LED integrato ESP32-C6
// LED_STATUS patterns:
//   DISARMED  → lampeggio lento (2s)
//   ARMED     → lampeggio veloce (200ms)
//   ALERT     → lampeggio rapidissimo (50ms)
//   ALARM     → fisso acceso

// ------------------------------------------------------------
// PIN BUZZER (opzionale - attivo se ENABLE_BUZZER=1)
// ------------------------------------------------------------
#define BUZZER_PIN          10
#define ENABLE_BUZZER       0       // 0=disabilitato, 1=abilitato

// ------------------------------------------------------------
// WIFI
// ------------------------------------------------------------
#define WIFI_SSID           "TUO_SSID"
#define WIFI_PASSWORD       "TUA_PASSWORD"
#define WIFI_TIMEOUT_MS     30000
#define WIFI_HOSTNAME       "autoguard"

// ------------------------------------------------------------
// MQTT
// ------------------------------------------------------------
#define MQTT_BROKER         "192.168.1.10"
#define MQTT_PORT           1883
#define MQTT_USER           ""
#define MQTT_PASS           ""
#define MQTT_CLIENT_ID      "autoguard-001"
#define MQTT_RECONNECT_MS   5000

// Topics publish (ESP32 → broker)
#define MQTT_TOPIC_STATUS   "autoguard/status"      // stato sistema
#define MQTT_TOPIC_SENSOR   "autoguard/sensor"      // dati radar (JSON)
#define MQTT_TOPIC_ALERT    "autoguard/alert"       // evento allarme
#define MQTT_TOPIC_LOG      "autoguard/log"         // log eventi

// Topics subscribe (broker → ESP32)
#define MQTT_TOPIC_CMD      "autoguard/command"     // arm/disarm/reset
#define MQTT_TOPIC_CFG      "autoguard/config"      // modifica soglie

// ------------------------------------------------------------
// RADAR HLK-LD2420 - Parametri rilevamento
// ------------------------------------------------------------
#define RADAR_MIN_DIST_CM   20      // distanza minima rilevamento (cm)
#define RADAR_MAX_DIST_CM   400     // distanza massima rilevamento (cm)
#define RADAR_UPDATE_MS     50      // intervallo aggiornamento (20Hz)

// Soglie per antifurto auto:
// Zona 1 CRITICA: 0-100cm  (dentro l'abitacolo o sopra il cofano)
// Zona 2 MEDIA:   100-250cm (intorno all'auto)
// Zona 3 BASSA:   250-400cm (nei pressi dell'auto)
#define ZONE_CRITICAL_MAX   100
#define ZONE_MEDIUM_MAX     250
#define ZONE_FAR_MAX        400

// ------------------------------------------------------------
// ALARM LOGIC - Temporizzazioni
// ------------------------------------------------------------
#define ARMING_DELAY_MS     5000    // ritardo armamento dopo comando (5s)
#define PRE_ALARM_MS        3000    // warning prima di scattare allarme (3s)
#define ALARM_DURATION_MS   30000   // durata allarme (30s)
#define COOLDOWN_MS         10000   // pausa tra allarmi consecutivi (10s)

// Numero rilevamenti consecutivi per confermare presenza
#define DETECTIONS_TO_ALERT 5       // 5 rilevamenti @ 20Hz = 250ms

// ------------------------------------------------------------
// WEB SERVER
// ------------------------------------------------------------
#define WEB_SERVER_PORT     80
#define OTA_PASSWORD        "autoguard"

// ------------------------------------------------------------
// NVS (Salvataggio impostazioni in flash)
// ------------------------------------------------------------
#define NVS_NAMESPACE       "autoguard"

#endif // CONFIG_H
