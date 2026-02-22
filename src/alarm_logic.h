// ============================================================
// AutoGuard - Alarm Logic - State Machine
// ============================================================
#ifndef ALARM_LOGIC_H
#define ALARM_LOGIC_H

#include <Arduino.h>
#include "config.h"
#include "sensor_ld2420.h"

// ------------------------------------------------------------
// Stati del sistema
// ------------------------------------------------------------
enum AlarmState {
    STATE_DISARMED  = 0,  // disarmato - sensore inattivo
    STATE_ARMING    = 1,  // armamento in corso (countdown 5s)
    STATE_ARMED     = 2,  // armato - sensore attivo
    STATE_ALERT     = 3,  // presenza rilevata - pre-allarme
    STATE_ALARM     = 4,  // allarme attivo
    STATE_COOLDOWN  = 5   // pausa post-allarme
};

// ------------------------------------------------------------
// Evento allarme (per log e MQTT)
// ------------------------------------------------------------
struct AlarmEvent {
    AlarmState  state;          // stato corrente
    AlarmState  prevState;      // stato precedente
    RadarZone   zone;           // zona rilevamento
    int         distance_cm;    // distanza rilevata
    uint32_t    timestamp;      // millis() evento
    bool        isNew;          // true = evento nuovo da inviare
};

// ------------------------------------------------------------
// Classe AlarmLogic
// ------------------------------------------------------------
class AlarmLogic {
public:
    AlarmLogic();

    // Inizializza
    void begin();

    // Aggiorna state machine (chiamare nel loop)
    void update(RadarData& radarData);

    // Comandi esterni (web/MQTT)
    void arm();
    void disarm();
    void reset();

    // Getters
    AlarmState  getState();
    const char* getStateName();
    const char* getStateName(AlarmState state);
    AlarmEvent  getLastEvent();
    uint32_t    getStateElapsedMs();    // ms trascorsi nello stato corrente
    uint32_t    getArmingCountdown();   // secondi al termine armamento
    uint32_t    getAlarmElapsedMs();    // ms dall'inizio allarme

    // true se c'è un evento nuovo da processare
    bool hasNewEvent();
    void clearNewEvent();

private:
    AlarmState  _state;
    AlarmState  _prevState;
    AlarmEvent  _lastEvent;
    uint32_t    _stateStartMs;      // millis() ingresso stato corrente
    uint32_t    _lastDetectionMs;   // millis() ultimo rilevamento

    // Transizioni stati
    void _setState(AlarmState newState, RadarData* data = nullptr);

    // Handler per ogni stato
    void _handleDisarmed(RadarData& data);
    void _handleArming(RadarData& data);
    void _handleArmed(RadarData& data);
    void _handleAlert(RadarData& data);
    void _handleAlarm(RadarData& data);
    void _handleCooldown(RadarData& data);

    // Azioni
    void _activateAlarm();
    void _deactivateAlarm();
};

#endif // ALARM_LOGIC_H
