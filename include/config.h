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

// ------------------------------------------------------------
// PIN BUZZER (opzionale)
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
#define MQTT_TOPIC_STATUS   "autoguard/status"
#define MQTT_TOPIC_SENSOR   "autoguard/sensor"
#define MQTT_TOPIC_ALERT    "autoguard/alert"
#define MQTT_TOPIC_LOG      "autoguard/log"

// Topics subscribe (broker → ESP32)
#define MQTT_TOPIC_CMD      "autoguard/command"
#define MQTT_TOPIC_CFG      "autoguard/config"

// ------------------------------------------------------------
// RADAR HLK-LD2420
// ------------------------------------------------------------
#define RADAR_MIN_DIST_CM   20
#define RADAR_MAX_DIST_CM   400
#define RADAR_UPDATE_MS     50      // 20Hz

// Zone rilevamento
#define ZONE_CRITICAL_MAX   100     // 0-100cm:   dentro/sopra auto
#define ZONE_MEDIUM_MAX     250     // 100-250cm: intorno auto
#define ZONE_FAR_MAX        400     // 250-400cm: nei pressi auto

// ------------------------------------------------------------
// ALARM LOGIC
// ------------------------------------------------------------
#define ARMING_DELAY_MS     5000    // ritardo armamento (5s)
#define PRE_ALARM_MS        3000    // warning pre-allarme (3s)
#define ALARM_DURATION_MS   30000   // durata allarme (30s)
#define COOLDOWN_MS         10000   // pausa tra allarmi (10s)
#define DETECTIONS_TO_ALERT 5       // rilevamenti consecutivi per alert

// ------------------------------------------------------------
// WEB SERVER
// ------------------------------------------------------------
#define WEB_SERVER_PORT     80
#define OTA_PASSWORD        "autoguard"

// ------------------------------------------------------------
// NVS
// ------------------------------------------------------------
#define NVS_NAMESPACE       "autoguard"

#endif // CONFIG_H
