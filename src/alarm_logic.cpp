// ============================================================
// AutoGuard - Alarm Logic - Implementazione
// ============================================================
#include "alarm_logic.h"

// ============================================================
// Costruttore
// ============================================================
AlarmLogic::AlarmLogic() :
    _state(STATE_DISARMED),
    _prevState(STATE_DISARMED),
    _stateStartMs(0),
    _lastDetectionMs(0)
{
    _lastEvent.state       = STATE_DISARMED;
    _lastEvent.prevState   = STATE_DISARMED;
    _lastEvent.zone        = ZONE_NONE;
    _lastEvent.distance_cm = 0;
    _lastEvent.timestamp   = 0;
    _lastEvent.isNew       = false;
}

// ============================================================
// begin()
// ============================================================
void AlarmLogic::begin() {
    _state        = STATE_DISARMED;
    _stateStartMs = millis();
    Serial.println("[ALARM] State machine inizializzata - DISARMED");
}

// ============================================================
// update() - Chiamare nel loop()
// ============================================================
void AlarmLogic::update(RadarData& radarData) {
    switch (_state) {
        case STATE_DISARMED: _handleDisarmed(radarData); break;
        case STATE_ARMING:   _handleArming(radarData);   break;
        case STATE_ARMED:    _handleArmed(radarData);    break;
        case STATE_ALERT:    _handleAlert(radarData);    break;
        case STATE_ALARM:    _handleAlarm(radarData);    break;
        case STATE_COOLDOWN: _handleCooldown(radarData); break;
    }
}

// ============================================================
// Comandi esterni
// ============================================================
void AlarmLogic::arm() {
    if (_state == STATE_DISARMED) {
        Serial.println("[ALARM] Comando ARM ricevuto");
        _setState(STATE_ARMING);
    } else {
        Serial.printf("[ALARM] Comando ARM ignorato (stato: %s)\n",
            getStateName());
    }
}

void AlarmLogic::disarm() {
    if (_state != STATE_DISARMED) {
        Serial.println("[ALARM] Comando DISARM ricevuto");
        _deactivateAlarm();
        _setState(STATE_DISARMED);
    } else {
        Serial.println("[ALARM] Già disarmato");
    }
}

void AlarmLogic::reset() {
    if (_state == STATE_ALARM || _state == STATE_COOLDOWN) {
        Serial.println("[ALARM] Comando RESET ricevuto");
        _deactivateAlarm();
        _setState(STATE_ARMED);
    } else {
        Serial.printf("[ALARM] Comando RESET ignorato (stato: %s)\n",
            getStateName());
    }
}

// ============================================================
// Handler DISARMED
// ============================================================
void AlarmLogic::_handleDisarmed(RadarData& data) {
    // Niente da fare - attende comando arm()
}

// ============================================================
// Handler ARMING - countdown prima di armarsi
// ============================================================
void AlarmLogic::_handleArming(RadarData& data) {
    uint32_t elapsed = millis() - _stateStartMs;

    // Stampa countdown ogni secondo
    static uint32_t lastPrint = 0;
    if (millis() - lastPrint > 1000) {
        lastPrint = millis();
        uint32_t remaining = (elapsed < ARMING_DELAY_MS) ? (ARMING_DELAY_MS - elapsed) / 1000 : 0;
        Serial.printf("[ALARM] Armamento in %lu secondi...\n", remaining);
    }

    // Countdown terminato -> passa ad ARMED
    if (elapsed >= ARMING_DELAY_MS) {
        Serial.println("[ALARM] Sistema ARMATO!");
        _setState(STATE_ARMED);
    }
}

// ============================================================
// Handler ARMED - monitora rilevamenti
// ============================================================
void AlarmLogic::_handleArmed(RadarData& data) {
    if (!data.detected) return;

    // Presenza rilevata - aggiorna timestamp
    _lastDetectionMs = millis();

    // Controlla zona e rilevamenti consecutivi
    // Zona CRITICA: scatta subito alert
    // Zona MEDIA/FAR: richiede N rilevamenti consecutivi
    bool triggerAlert = false;

    if (data.zone == ZONE_CRITICAL) {
        Serial.printf("[ALARM] ZONA CRITICA! Dist:%dcm\n", data.distance_cm);
        triggerAlert = true;
    } else if (data.zone == ZONE_MEDIUM || data.zone == ZONE_FAR) {
        // Richiedi DETECTIONS_TO_ALERT rilevamenti consecutivi
        // Il contatore è gestito da SensorLD2420
        // Qui leggiamo direttamente i dati
        static int consecCount = 0;
        consecCount++;
        if (consecCount >= DETECTIONS_TO_ALERT) {
            Serial.printf("[ALARM] Presenza confermata! Zona:%d Dist:%dcm\n",
                data.zone, data.distance_cm);
            triggerAlert = true;
            consecCount = 0;
        }
    }

    if (triggerAlert) {
        _setState(STATE_ALERT, &data);
    }
}

// ============================================================
// Handler ALERT - pre-allarme, attende conferma
// ============================================================
void AlarmLogic::_handleAlert(RadarData& data) {
    uint32_t elapsed = millis() - _stateStartMs;

    // Stampa warning ogni 500ms
    static uint32_t lastWarn = 0;
    if (millis() - lastWarn > 500) {
        lastWarn = millis();
        Serial.printf("[ALARM] ⚠ PRE-ALLARME! Scatto in %lums\n",
            elapsed < PRE_ALARM_MS ? PRE_ALARM_MS - elapsed : 0);
    }

    // Se la presenza scompare durante pre-allarme -> torna ARMED
    if (!data.detected) {
        uint32_t noDetectTime = millis() - _lastDetectionMs;
        if (noDetectTime > 2000) {  // 2s senza rilevamento
            Serial.println("[ALARM] Presenza scomparsa - torno ad ARMED");
            _setState(STATE_ARMED);
            return;
        }
    } else {
        _lastDetectionMs = millis();
    }

    // Timeout pre-allarme -> scatta allarme
    if (elapsed >= PRE_ALARM_MS) {
        Serial.println("[ALARM] 🚨 ALLARME ATTIVATO!");
        _activateAlarm();
        _setState(STATE_ALARM, &data);
    }
}

// ============================================================
// Handler ALARM - allarme attivo
// ============================================================
void AlarmLogic::_handleAlarm(RadarData& data) {
    uint32_t elapsed = millis() - _stateStartMs;

    // Stampa stato ogni 5s
    static uint32_t lastPrint = 0;
    if (millis() - lastPrint > 5000) {
        lastPrint = millis();
        Serial.printf("[ALARM] 🚨 ALLARME ATTIVO da %lus\n", elapsed / 1000);
    }

    // Timeout allarme -> passa a COOLDOWN
    if (elapsed >= ALARM_DURATION_MS) {
        Serial.println("[ALARM] Timeout allarme - COOLDOWN");
        _deactivateAlarm();
        _setState(STATE_COOLDOWN);
    }
}

// ============================================================
// Handler COOLDOWN - pausa post-allarme
// ============================================================
void AlarmLogic::_handleCooldown(RadarData& data) {
    uint32_t elapsed = millis() - _stateStartMs;

    // Stampa stato ogni 5s
    static uint32_t lastPrint = 0;
    if (millis() - lastPrint > 5000) {
        lastPrint = millis();
        uint32_t remaining = (elapsed < COOLDOWN_MS) ? (COOLDOWN_MS - elapsed) / 1000 : 0;
        Serial.printf("[ALARM] Cooldown: %lus rimanenti\n", remaining);
    }

    // Cooldown terminato -> torna ARMED
    if (elapsed >= COOLDOWN_MS) {
        Serial.println("[ALARM] Cooldown terminato - torno ad ARMED");
        _setState(STATE_ARMED);
    }
}

// ============================================================
// _setState() - Transizione di stato
// ============================================================
void AlarmLogic::_setState(AlarmState newState, RadarData* data) {
    _prevState    = _state;
    _state        = newState;
    _stateStartMs = millis();

    Serial.printf("[ALARM] Transizione: %s -> %s\n",
        getStateName(_prevState),
        getStateName(_state));

    // Registra evento
    _lastEvent.state       = _state;
    _lastEvent.prevState   = _prevState;
    _lastEvent.zone        = data ? data->zone : ZONE_NONE;
    _lastEvent.distance_cm = data ? data->distance_cm : 0;
    _lastEvent.timestamp   = millis();
    _lastEvent.isNew       = true;
}

// ============================================================
// _activateAlarm() - Attiva uscite allarme
// ============================================================
void AlarmLogic::_activateAlarm() {
    digitalWrite(LED_STATUS_PIN, HIGH);

#if ENABLE_BUZZER
    // PWM buzzer
    ledcAttach(BUZZER_PIN, 2000, 8);
    ledcWrite(BUZZER_PIN, 128);
#endif

    Serial.println("[ALARM] Uscite allarme ATTIVATE");
}

// ============================================================
// _deactivateAlarm() - Disattiva uscite allarme
// ============================================================
void AlarmLogic::_deactivateAlarm() {
    digitalWrite(LED_STATUS_PIN, LOW);

#if ENABLE_BUZZER
    ledcDetach(BUZZER_PIN);
#endif

    Serial.println("[ALARM] Uscite allarme DISATTIVATE");
}

// ============================================================
// Getters
// ============================================================
AlarmState AlarmLogic::getState() {
    return _state;
}

const char* AlarmLogic::getStateName() {
    return getStateName(_state);
}

const char* AlarmLogic::getStateName(AlarmState state) {
    switch (state) {
        case STATE_DISARMED: return "DISARMED";
        case STATE_ARMING:   return "ARMING";
        case STATE_ARMED:    return "ARMED";
        case STATE_ALERT:    return "ALERT";
        case STATE_ALARM:    return "ALARM";
        case STATE_COOLDOWN: return "COOLDOWN";
        default:             return "UNKNOWN";
    }
}

AlarmEvent AlarmLogic::getLastEvent() {
    return _lastEvent;
}

uint32_t AlarmLogic::getStateElapsedMs() {
    return millis() - _stateStartMs;
}

uint32_t AlarmLogic::getArmingCountdown() {
    if (_state != STATE_ARMING) return 0;
    uint32_t elapsed = millis() - _stateStartMs;
    return elapsed < ARMING_DELAY_MS ? (ARMING_DELAY_MS - elapsed) : 0;
}

uint32_t AlarmLogic::getAlarmElapsedMs() {
    if (_state != STATE_ALARM) return 0;
    return millis() - _stateStartMs;
}

bool AlarmLogic::hasNewEvent() {
    return _lastEvent.isNew;
}

void AlarmLogic::clearNewEvent() {
    _lastEvent.isNew = false;
}
