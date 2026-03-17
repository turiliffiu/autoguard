// ============================================================
// AutoGuard - Config Manager (NVS) - Implementazione
// ============================================================
#include "config_manager.h"

// Istanza globale
ConfigManager configMgr;

// Chiavi NVS
#define KEY_ZONE_CRIT    "zone_crit"
#define KEY_ZONE_MED     "zone_med"
#define KEY_ZONE_FAR     "zone_far"
#define KEY_ARM_DELAY    "arm_delay"
#define KEY_PRE_ALARM    "pre_alarm"
#define KEY_ALARM_DUR    "alarm_dur"
#define KEY_COOLDOWN     "cooldown"
#define KEY_DETECTIONS   "detections"
#define KEY_RADAR_MIN    "radar_min"
#define KEY_RADAR_MAX    "radar_max"

ConfigManager::ConfigManager() {
    _loadDefaults();
}

void ConfigManager::_loadDefaults() {
    _cfg.zoneCriticalMax   = ZONE_CRITICAL_MAX;
    _cfg.zoneMediumMax     = ZONE_MEDIUM_MAX;
    _cfg.zoneFarMax        = ZONE_FAR_MAX;
    _cfg.armingDelayMs     = ARMING_DELAY_MS;
    _cfg.preAlarmMs        = PRE_ALARM_MS;
    _cfg.alarmDurationMs   = ALARM_DURATION_MS;
    _cfg.cooldownMs        = COOLDOWN_MS;
    _cfg.detectionsToAlert = DETECTIONS_TO_ALERT;
    _cfg.radarMinDist      = RADAR_MIN_DIST_CM;
    _cfg.radarMaxDist      = RADAR_MAX_DIST_CM;
}

void ConfigManager::begin() {
    _prefs.begin(NVS_NAMESPACE, false);

    // Carica da NVS, usa default se non trovato
    _cfg.zoneCriticalMax   = _prefs.getInt(KEY_ZONE_CRIT,  _cfg.zoneCriticalMax);
    _cfg.zoneMediumMax     = _prefs.getInt(KEY_ZONE_MED,   _cfg.zoneMediumMax);
    _cfg.zoneFarMax        = _prefs.getInt(KEY_ZONE_FAR,   _cfg.zoneFarMax);
    _cfg.armingDelayMs     = _prefs.getInt(KEY_ARM_DELAY,  _cfg.armingDelayMs);
    _cfg.preAlarmMs        = _prefs.getInt(KEY_PRE_ALARM,  _cfg.preAlarmMs);
    _cfg.alarmDurationMs   = _prefs.getInt(KEY_ALARM_DUR,  _cfg.alarmDurationMs);
    _cfg.cooldownMs        = _prefs.getInt(KEY_COOLDOWN,   _cfg.cooldownMs);
    _cfg.detectionsToAlert = _prefs.getInt(KEY_DETECTIONS, _cfg.detectionsToAlert);
    _cfg.radarMinDist      = _prefs.getInt(KEY_RADAR_MIN,  _cfg.radarMinDist);
    _cfg.radarMaxDist      = _prefs.getInt(KEY_RADAR_MAX,  _cfg.radarMaxDist);

    _prefs.end();

    Serial.println("[CFG] Configurazione caricata da NVS");
    print();
}

bool ConfigManager::save(const AutoGuardConfig& cfg) {
    _cfg = cfg;

    _prefs.begin(NVS_NAMESPACE, false);
    _prefs.putInt(KEY_ZONE_CRIT,  cfg.zoneCriticalMax);
    _prefs.putInt(KEY_ZONE_MED,   cfg.zoneMediumMax);
    _prefs.putInt(KEY_ZONE_FAR,   cfg.zoneFarMax);
    _prefs.putInt(KEY_ARM_DELAY,  cfg.armingDelayMs);
    _prefs.putInt(KEY_PRE_ALARM,  cfg.preAlarmMs);
    _prefs.putInt(KEY_ALARM_DUR,  cfg.alarmDurationMs);
    _prefs.putInt(KEY_COOLDOWN,   cfg.cooldownMs);
    _prefs.putInt(KEY_DETECTIONS, cfg.detectionsToAlert);
    _prefs.putInt(KEY_RADAR_MIN,  cfg.radarMinDist);
    _prefs.putInt(KEY_RADAR_MAX,  cfg.radarMaxDist);
    _prefs.end();

    Serial.println("[CFG] Configurazione salvata in NVS");
    print();
    return true;
}

AutoGuardConfig ConfigManager::get() {
    return _cfg;
}

void ConfigManager::resetDefaults() {
    _loadDefaults();
    save(_cfg);
    Serial.println("[CFG] Reset ai valori di default");
}

void ConfigManager::print() {
    Serial.println("[CFG] ---- Configurazione corrente ----");
    Serial.printf("[CFG] Zone: CRITICAL<%dcm MEDIUM<%dcm FAR<%dcm\n",
        _cfg.zoneCriticalMax, _cfg.zoneMediumMax, _cfg.zoneFarMax);
    Serial.printf("[CFG] Timing: ARM=%ds PRE=%ds ALARM=%ds COOL=%ds\n",
        _cfg.armingDelayMs/1000, _cfg.preAlarmMs/1000,
        _cfg.alarmDurationMs/1000, _cfg.cooldownMs/1000);
    Serial.printf("[CFG] Sensibilita: detect=%d radarMin=%dcm radarMax=%dcm\n",
        _cfg.detectionsToAlert, _cfg.radarMinDist, _cfg.radarMaxDist);
    Serial.println("[CFG] ------------------------------------");
}
