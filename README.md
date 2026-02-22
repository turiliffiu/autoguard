# 🚗 AutoGuard
## Antifurto Auto con ESP32-C6 + HLK-LD2420 mmWave Radar

### Hardware
- ESP32-C6 DevKit
- HLK-LD2420 Radar 24GHz

### Cablaggio
| ESP32-C6 | HLK-LD2420 |
|----------|------------|
| 3.3V     | J2P1 (VCC) |
| GND      | J2P2 (GND) |
| GPIO4    | J2P3 (OT1/TX) |
| GPIO5    | J2P4 (OT2/RX) |

### Build
```bash
pio run -e debug
pio run --target upload
pio device monitor
```

### MQTT Topics
| Topic | Direzione | Contenuto |
|-------|-----------|-----------|
| autoguard/status | publish | stato sistema |
| autoguard/sensor | publish | dati radar JSON |
| autoguard/alert  | publish | evento allarme |
| autoguard/command | subscribe | arm/disarm/reset |

### Versione
1.0.0 - Setup iniziale
