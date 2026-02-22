# 🚗 AutoGuard

**Sistema antifurto intelligente per auto** basato su **ESP32-C6** e radar millimetrico **HLK-LD2420 24GHz**.

Rileva presenze nell'area intorno all'auto tramite radar FMCW, gestisce una state machine di allarme multi-livello e si integra nativamente con **Home Assistant** via MQTT Discovery.

---

## ✨ Features

### Implementate
- ✅ **Rilevamento radar** HLK-LD2420 24GHz FMCW (range 0.2–8m, 20Hz)
- ✅ **3 zone di rilevamento** configurabili (Critica / Media / Lontana)
- ✅ **State machine** multi-stato: DISARMED → ARMING → ARMED → ALERT → ALARM → COOLDOWN
- ✅ **Web dashboard** responsive (http://IP_ESP32) con controlli arm/disarm
- ✅ **API REST** (`/api/status`, `/api/arm`, `/api/disarm`, `/api/reset`)
- ✅ **MQTT client** con publish stato, radar, alert
- ✅ **MQTT Discovery** Home Assistant — crea automaticamente dispositivo e entità
- ✅ **LED di stato** con pattern blink diversi per ogni stato
- ✅ **Comandi seriali** per debug (a/d/r/s)

### Roadmap
- 🔜 **OTA** aggiornamento firmware via browser
- 🔜 **NVS** salvataggio stato arm/disarm su riavvio
- 🔜 **Notifiche Telegram** quando scatta l'allarme
- 🔜 **Buzzer PWM** (pin già configurato)
- 🔜 **Automazioni HA** esempi pronti all'uso
- 🔜 **Calibrazione radar** via web dashboard
- 🔜 **Multi-zona** supporto più sensori

---

## 🏗️ Architettura
```
┌─────────────────────────────────────────────────┐
│              Home Assistant                     │
│   MQTT Discovery → Dispositivo AutoGuard        │
│   Entità: stato, distanza, presenza, allarme    │
│   Controlli: Arma, Disarma, Reset               │
└────────────────────┬────────────────────────────┘
                     │ MQTT
┌────────────────────▼────────────────────────────┐
│             ESP32-C6 DevKit                     │
│                                                 │
│  ┌──────────┐  ┌──────────┐  ┌───────────────┐  │
│  │ Alarm    │  │ Web      │  │ MQTT          │  │
│  │ Logic    │  │ Server   │  │ Client        │  │
│  │ (SM)     │  │ :80      │  │ + Discovery   │  │
│  └────┬─────┘  └──────────┘  └───────────────┘  │
│       │                                         │
│  ┌────▼─────────────────────────────────────┐   │
│  │         Driver HLK-LD2420                │   │
│  │   UART1 | GPIO4(RX) GPIO5(TX) | 115200   │   │
│  └──────────────────────────────────────────┘   │
└─────────────────────┬───────────────────────────┘
                      │ UART
┌─────────────────────▼───────────────────────────┐
│           HLK-LD2420 Radar 24GHz                │
│         Range: 0.2–8m | FMCW | 3.3V             │
└─────────────────────────────────────────────────┘
```

---

## 🔧 Hardware

### Componenti
| Componente | Modello | Note |
|------------|---------|------|
| Microcontroller | ESP32-C6 DevKit | WiFi 6, BT 5, 160MHz |
| Radar | HLK-LD2420 | 24GHz FMCW, 0.2-8m |
| LED | Integrato GPIO8 | Active-low su ESP32-C6 |
| Buzzer | Qualsiasi 3.3V | Opzionale, GPIO10 |

### Cablaggio HLK-LD2420 → ESP32-C6
```
HLK-LD2420          ESP32-C6
──────────          ────────
J2P1 (3.3V)   →    3.3V
J2P2 (GND)    →    GND
J2P3 (TX/OT1) →    GPIO4 (RX)
J2P4 (RX/OT2) →    GPIO5 (TX)
```

---

## 📊 State Machine
```
                    ┌──────────────┐
               ┌───►│   DISARMED   │◄──────────────┐
               │    └──────┬───────┘               │
               │           │ arm()                 │
               │    ┌──────▼───────┐               │
               │    │   ARMING     │ disarm()──────┤
               │    │  (5s delay)  │               │
               │    └──────┬───────┘               │
               │           │ timeout               │
               │    ┌──────▼───────┐               │
         disarm│    │    ARMED     │ disarm()──────┤
               │    └──────┬───────┘               │
               │           │ rilevamento           │
               │    ┌──────▼───────┐               │
               │    │    ALERT     │               │
               │    │  (3s warn)   │               │
               │    └──────┬───────┘               │
               │           │ conferma              │
               │    ┌──────▼───────┐               │
               │    │    ALARM     │               │
               │    │  (30s)       │               │
               │    └──────┬───────┘               │
               │           │ timeout               │
               │    ┌──────▼───────┐               │
               └────│   COOLDOWN   │               │
                    │  (10s)       │───────────────┘
                    └─────────────-┘
```

### Zone radar
| Zona | Range | Comportamento |
|------|-------|---------------|
| CRITICA | 0–100cm | Alert immediato |
| MEDIA | 100–250cm | Alert dopo N rilevamenti |
| LONTANA | 250–400cm | Alert dopo N rilevamenti |

---

## 🏠 Integrazione Home Assistant

Il dispositivo si registra automaticamente via **MQTT Discovery**.

### Entità create automaticamente
| Entità | Tipo | Descrizione |
|--------|------|-------------|
| `sensor.autoguard_stato` | Sensor | Stato corrente sistema |
| `sensor.autoguard_distanza_radar` | Sensor | Distanza rilevata (cm) |
| `binary_sensor.autoguard_presenza_rilevata` | Binary Sensor | Presenza ON/OFF |
| `binary_sensor.autoguard_allarme` | Binary Sensor | Allarme attivo |
| `button.autoguard_arma` | Button | Arma il sistema |
| `button.autoguard_disarma` | Button | Disarma il sistema |
| `button.autoguard_reset_allarme` | Button | Reset allarme |

### Topic MQTT
```
autoguard/status    ← Stato sistema (JSON, retain)
autoguard/sensor    ← Dati radar (JSON, ogni 10s)
autoguard/alert     ← Eventi allarme (JSON)
autoguard/command   → Comandi (arm/disarm/reset/status)
```

---

## 🚀 Installazione

### Prerequisiti
- Python 3.8+
- Git
- ESP32-C6 DevKit connesso via USB

### 1. Clone repository
```bash
git clone https://github.com/turiliffiu/autoguard.git
cd autoguard
```

### 2. Ambiente virtuale Python
```bash
python3 -m venv venv
source venv/bin/activate        # Linux/macOS
# venv\Scripts\activate         # Windows
pip install platformio
```

### 3. Configurazione
Modifica `include/config.h` con i tuoi parametri:
```cpp
// WiFi
#define WIFI_SSID       "TUO_SSID"
#define WIFI_PASSWORD   "TUA_PASSWORD"

// MQTT (IP del tuo Home Assistant)
#define MQTT_BROKER     "192.168.1.XXX"
#define MQTT_USER       "mqtt_user"
#define MQTT_PASS       "mqtt_password"

// Zone radar (cm) - calibra dopo installazione
#define ZONE_CRITICAL_MAX   100
#define ZONE_MEDIUM_MAX     250
#define ZONE_FAR_MAX        400
```

### 4. Compilazione
```bash
# Debug (con log verbose)
pio run -e debug

# Release (ottimizzato)
pio run -e release
```

### 5. Upload firmware
```bash
# Via USB (prima volta)
pio run -e debug --target upload

# Monitor seriale
pio device monitor
```

### 6. Verifica
Apri il browser su `http://IP_ESP32` — la dashboard deve mostrare lo stato DISARMED.

In Home Assistant → Impostazioni → Dispositivi → MQTT → trovi il dispositivo **AutoGuard**.

---

## 🖥️ Setup Server di Sviluppo Linux

Per sviluppo su server Linux (Ubuntu/Debian) senza display.

### Installazione dipendenze
```bash
# Aggiorna sistema
sudo apt update && sudo apt upgrade -y

# Python e Git
sudo apt install -y python3 python3-pip python3-venv git

# Regole udev per ESP32 (necessario per upload USB)
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core/develop/platformio/assets/system/99-platformio-udev.rules \
  | sudo tee /etc/udev/rules.d/99-platformio-udev.rules
sudo udevadm control --reload-rules
sudo udevadm trigger
sudo usermod -a -G dialout $USER
sudo usermod -a -G plugdev $USER
# ⚠️ Logout e login per applicare i gruppi
```

### Clone e setup
```bash
git clone https://github.com/turiliffiu/autoguard.git
cd autoguard
python3 -m venv venv
source venv/bin/activate
pip install platformio
```

### Alias utili (aggiungi a ~/.bashrc)
```bash
alias autoguard-dev='cd ~/autoguard && source venv/bin/activate'
alias autoguard-build='pio run -e debug'
alias autoguard-upload='pio run -e debug --target upload'
alias autoguard-monitor='pio device monitor'
alias autoguard-all='pio run -e debug --target upload && pio device monitor'
```

### Workflow sviluppo
```bash
# 1. Attiva ambiente
autoguard-dev

# 2. Modifica codice
nano src/qualcosa.cpp

# 3. Compila
autoguard-build

# 4. Carica su ESP32 (via USB)
autoguard-upload

# 5. Monitora output seriale
autoguard-monitor
# Comandi nel monitor: a=arm  d=disarm  r=reset  s=status

# 6. Commit modifiche
git add . && git commit -m "descrizione modifica"
git push
```

### Deploy da GitHub su nuovo server
```bash
# Clone fresco
git clone https://github.com/turiliffiu/autoguard.git
cd autoguard

# Setup ambiente
python3 -m venv venv && source venv/bin/activate
pip install platformio

# Prima build (scarica toolchain ~500MB, solo prima volta)
pio run -e debug

# Upload
pio run -e debug --target upload
```

---

## 🗂️ Struttura Progetto
```
autoguard/
├── src/
│   ├── main.cpp              # Entry point, loop principale
│   ├── sensor_ld2420.h/.cpp  # Driver radar HLK-LD2420
│   ├── alarm_logic.h/.cpp    # State machine antifurto
│   ├── web_server.h/.cpp     # Dashboard web + API REST
│   └── mqtt_client.h/.cpp    # MQTT + HA Discovery
├── include/
│   └── config.h              # ⚙️ Configurazione centrale
├── platformio.ini            # Build environments
└── README.md
```

---

## ⚙️ Configurazione Avanzata

### Timing allarme (`include/config.h`)
```cpp
#define ARMING_DELAY_MS      5000   // Ritardo armamento (5s)
#define PRE_ALARM_MS         3000   // Pre-allarme prima di ALARM (3s)
#define ALARM_DURATION_MS   30000   // Durata allarme (30s)
#define COOLDOWN_MS         10000   // Pausa tra allarmi (10s)
#define DETECTIONS_TO_ALERT     5   // Rilevamenti consecutivi per alert
```

### Calibrazione zone radar
```cpp
#define ZONE_CRITICAL_MAX   100   // cm - zona critica (dentro/sopra auto)
#define ZONE_MEDIUM_MAX     250   // cm - zona media (intorno auto)
#define ZONE_FAR_MAX        400   // cm - zona lontana (nei pressi)
```

### Environments PlatformIO
| Environment | Uso |
|-------------|-----|
| `debug` | Sviluppo, log verbose |
| `release` | Produzione, ottimizzato |
| `ota` | Upload wireless (dopo prima installazione) |

---

## 🐛 Troubleshooting

### ESP32 non trovato su porta USB
```bash
# Verifica porta
ls /dev/ttyACM* /dev/ttyUSB*

# Reinstalla udev rules
sudo udevadm control --reload-rules && sudo udevadm trigger

# Verifica gruppi utente
groups $USER  # deve includere dialout e plugdev
```

### MQTT non si connette
- Verifica IP broker in `config.h`
- Verifica credenziali MQTT in `config.h`
- Controlla che il broker Mosquitto sia attivo in HA
- Verifica firewall sulla porta 1883

### Discovery HA non appare
- Verifica che l'integrazione MQTT sia attiva in HA
- Controlla Impostazioni → Dispositivi → MQTT
- Riavvia ESP32 per ripubblicare la discovery

### Crash al boot (nessun output seriale)
- Oggetti complessi (`AsyncWebServer`, `PubSubClient`) devono essere **puntatori** inizializzati in `setup()`, non variabili globali

---

## 📝 Changelog

### v1.0.0 (2026-02-22)
- ✅ Driver HLK-LD2420 con filtro media mobile e 3 zone
- ✅ State machine completa 6 stati
- ✅ Web dashboard responsive con API REST
- ✅ MQTT client con publish/subscribe
- ✅ MQTT Discovery Home Assistant
- ✅ Fix: oggetti complessi come puntatori (crash pre-setup)
- ✅ Fix: LED active-low ESP32-C6
- ✅ Fix: overflow uint32_t countdown

---

## 📄 Licenza

MIT License — libero uso, modifica e distribuzione.

---

## 👤 Autore

**Salvo (turiliffiu)**
- GitHub: [@turiliffiu](https://github.com/turiliffiu)
- Repository: [autoguard](https://github.com/turiliffiu/autoguard)

---

*AutoGuard v1.0.0 — ESP32-C6 + HLK-LD2420 | Home Assistant Ready*
