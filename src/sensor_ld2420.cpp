// ============================================================
// AutoGuard - Driver HLK-LD2420 - Implementazione
// ============================================================
#include "sensor_ld2420.h"

// ============================================================
// Costruttore
// ============================================================
SensorLD2420::SensorLD2420() :
    _radarSerial(1),
    _ready(false),
    _consecutiveDetections(0),
    _filterIdx(0),
    _filterFull(false)
{
    // Inizializza buffer filtro a zero
    for (int i = 0; i < FILTER_SIZE; i++) {
        _filterBuf[i] = 0;
    }

    // Inizializza struttura dati
    _data.detected      = false;
    _data.distance_cm   = 0;
    _data.zone          = ZONE_NONE;
    _data.filtered_dist = 0;
    _data.timestamp     = 0;
}

// ============================================================
// begin() - Inizializza sensore
// ============================================================
bool SensorLD2420::begin() {
    Serial.println("[RADAR] Inizializzazione HLK-LD2420...");
    Serial.printf("[RADAR] RX=GPIO%d TX=GPIO%d BAUD=%d\n",
        RADAR_RX_PIN, RADAR_TX_PIN, RADAR_BAUD);

    // Inizializza UART1 con i pin configurati
    _radarSerial.begin(RADAR_BAUD, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);
    delay(100);

    // Inizializza libreria LD2420
    if (!_radar.begin(_radarSerial)) {
        Serial.println("[RADAR] ERRORE: sensore non risponde!");
        _ready = false;
        return false;
    }

    // Configura range di rilevamento
    _radar.setDistanceRange(RADAR_MIN_DIST_CM, RADAR_MAX_DIST_CM);

    // Configura intervallo aggiornamento
    _radar.setUpdateInterval(RADAR_UPDATE_MS);

    _ready = true;
    Serial.println("[RADAR] Inizializzazione OK!");
    Serial.printf("[RADAR] Range: %dcm - %dcm\n",
        RADAR_MIN_DIST_CM, RADAR_MAX_DIST_CM);

    return true;
}

// ============================================================
// update() - Aggiorna letture (chiamare nel loop)
// ============================================================
void SensorLD2420::update() {
    if (!_ready) return;

    // Aggiorna libreria
    _radar.update();

    bool nowDetected = _radar.isDetecting();
    int  rawDist     = _radar.getDistance();

    if (nowDetected) {
        // Applica filtro media mobile
        int filteredDist = _applyFilter(rawDist);

        // Aggiorna struttura dati
        _data.detected      = true;
        _data.distance_cm   = rawDist;
        _data.filtered_dist = filteredDist;
        _data.zone          = _getZone(filteredDist);
        _data.timestamp     = millis();

        // Incrementa contatore rilevamenti consecutivi
        _consecutiveDetections++;

#if DEBUG_MODE
        _printData();
#endif

    } else {
        // Nessun rilevamento
        _data.detected      = false;
        _data.distance_cm   = 0;
        _data.filtered_dist = 0;
        _data.zone          = ZONE_NONE;
        _data.timestamp     = millis();

        // Reset contatore
        _consecutiveDetections = 0;
    }
}

// ============================================================
// getData() - Restituisce ultimi dati
// ============================================================
RadarData SensorLD2420::getData() {
    return _data;
}

// ============================================================
// isReady() - Sensore inizializzato correttamente
// ============================================================
bool SensorLD2420::isReady() {
    return _ready;
}

// ============================================================
// getConsecutiveDetections() - Rilevamenti consecutivi
// ============================================================
int SensorLD2420::getConsecutiveDetections() {
    return _consecutiveDetections;
}

// ============================================================
// resetDetections() - Reset contatore
// ============================================================
void SensorLD2420::resetDetections() {
    _consecutiveDetections = 0;
}

// ============================================================
// _getZone() - Determina zona da distanza
// ============================================================
RadarZone SensorLD2420::_getZone(int distance_cm) {
    if (distance_cm <= 0) {
        return ZONE_NONE;
    } else if (distance_cm <= ZONE_CRITICAL_MAX) {
        return ZONE_CRITICAL;
    } else if (distance_cm <= ZONE_MEDIUM_MAX) {
        return ZONE_MEDIUM;
    } else if (distance_cm <= ZONE_FAR_MAX) {
        return ZONE_FAR;
    }
    return ZONE_NONE;
}

// ============================================================
// _applyFilter() - Media mobile su FILTER_SIZE campioni
// ============================================================
int SensorLD2420::_applyFilter(int newValue) {
    _filterBuf[_filterIdx] = newValue;
    _filterIdx = (_filterIdx + 1) % FILTER_SIZE;

    if (!_filterFull && _filterIdx == 0) {
        _filterFull = true;
    }

    int sum   = 0;
    int count = _filterFull ? FILTER_SIZE : _filterIdx;

    for (int i = 0; i < count; i++) {
        sum += _filterBuf[i];
    }

    return (count > 0) ? (sum / count) : 0;
}

// ============================================================
// _printData() - Stampa dati su Serial (solo debug)
// ============================================================
void SensorLD2420::_printData() {
    const char* zoneNames[] = {"NONE", "CRITICAL", "MEDIUM", "FAR"};

    Serial.printf("[RADAR] Dist:%dcm Filter:%dcm Zone:%s Consec:%d\n",
        _data.distance_cm,
        _data.filtered_dist,
        zoneNames[_data.zone],
        _consecutiveDetections);
}
