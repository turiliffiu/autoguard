// ============================================================
// AutoGuard - Driver HLK-LD2420
// ============================================================
#ifndef SENSOR_LD2420_H
#define SENSOR_LD2420_H

#include <Arduino.h>
#include <LD2420.h>
#include "config.h"

// Zone di rilevamento
enum RadarZone {
    ZONE_NONE     = 0,  // nessun rilevamento
    ZONE_CRITICAL = 1,  // 0-100cm:   dentro/sopra auto
    ZONE_MEDIUM   = 2,  // 100-250cm: intorno auto
    ZONE_FAR      = 3   // 250-400cm: nei pressi auto
};

// Dati restituiti dal sensore
struct RadarData {
    bool      detected;       // presenza rilevata
    int       distance_cm;    // distanza in cm
    RadarZone zone;           // zona rilevamento
    int       filtered_dist;  // distanza filtrata (media mobile)
    uint32_t  timestamp;      // millis() lettura
};

class SensorLD2420 {
public:
    SensorLD2420();

    // Inizializza sensore su Serial1 (GPIO4/5)
    bool begin();

    // Da chiamare nel loop() - aggiorna letture
    void update();

    // Restituisce gli ultimi dati letti
    RadarData getData();

    // true se il sensore è inizializzato correttamente
    bool isReady();

    // Conta rilevamenti consecutivi (per confermare presenza)
    int getConsecutiveDetections();

    // Reset contatore rilevamenti
    void resetDetections();

private:
    HardwareSerial  _radarSerial;
    LD2420          _radar;
    bool            _ready;
    RadarData       _data;
    int             _consecutiveDetections;

    // Filtro media mobile
    static const int FILTER_SIZE = 5;
    int  _filterBuf[FILTER_SIZE];
    int  _filterIdx;
    bool _filterFull;

    // Metodi interni
    RadarZone  _getZone(int distance_cm);
    int        _applyFilter(int newValue);
    void       _printData();
};

#endif // SENSOR_LD2420_H
