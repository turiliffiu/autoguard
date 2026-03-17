// ============================================================
// AutoGuard - Config Manager (NVS)
// ============================================================
#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

struct AutoGuardConfig {
    int zoneCriticalMax;
    int zoneMediumMax;
    int zoneFarMax;
    int armingDelayMs;
    int preAlarmMs;
    int alarmDurationMs;
    int cooldownMs;
    int detectionsToAlert;
    int radarMinDist;
    int radarMaxDist;
};

class ConfigManager {
public:
    ConfigManager();
    void begin();
    bool save(const AutoGuardConfig& cfg);
    AutoGuardConfig get();
    void resetDefaults();
    void print();

private:
    Preferences     _prefs;
    AutoGuardConfig _cfg;
    void _loadDefaults();
};

extern ConfigManager configMgr;

#endif // CONFIG_MANAGER_H
