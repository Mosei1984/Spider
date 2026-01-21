# 🕷️ Spider Robot

Ein vierbeiniger Spinnenroboter mit ESP8266 und 8 Servos, gesteuert über Web-Oberfläche oder ESP32 Hardware-Controller.

![ESP8266](https://img.shields.io/badge/ESP8266-Robot-blue)
![ESP32](https://img.shields.io/badge/ESP32-Remote-green)
![PlatformIO](https://img.shields.io/badge/PlatformIO-Arduino-orange)
![License](https://img.shields.io/badge/License-MIT-green)

## 🎯 Features

- **8 Servos** für 4 Beine (Schulter + Klaue pro Bein)
- **Web-Interface** mit Echtzeit-Steuerung via WebSocket
- **ESP32 Remote Controller** mit Joystick, Potis und OLED-Display
- **Bewegungen**: Vorwärts, Rückwärts, Seitwärts, Drehungen
- **Aktionen**: Stehen, Schlafen, Winken, Liegestütz, Tanzen, Kämpfen
- **Terrain-Modus**: Flach, Bergauf, Bergab
- **Kalibrierung**: Servo-Offsets über UI einstellbar und persistent gespeichert
- **Dual-Mode**: Robot als Access Point oder im Heimnetzwerk
- **Auto-Fallback**: Remote verbindet automatisch zu AP oder Heimnetz

## 📁 Projektstruktur

```
Spider/
├── src/                          # ESP8266 Robot Firmware
│   ├── main.cpp                  # Hauptprogramm
│   ├── motion/
│   │   └── MotionData.h/cpp      # Bewegungsmatrizen & Servo-Steuerung
│   ├── robot/
│   │   └── RobotController.h/cpp # Roboter-Logik & Befehlsverarbeitung
│   ├── web/
│   │   └── WebServer.h/cpp       # WebSocket & HTTP-Server
│   └── wifi/
│       └── WifiManager.h/cpp     # WLAN-Verbindung & AP-Modus
├── data/
│   └── index.html                # Web-Interface
├── SpiderRemote-ESP32/           # ESP32 Remote Controller
│   └── src/
│       ├── main.cpp              # Hauptprogramm Remote
│       ├── Config.h              # Konfiguration (WiFi, Pins, etc.)
│       ├── WsClient.h/cpp        # WebSocket Client
│       ├── DriveControl.h/cpp    # Joystick → Bewegungsbefehle
│       ├── Inputs.h/cpp          # Joystick & Poti Auswertung
│       ├── Buttons.h/cpp         # Button-Handling mit Debounce
│       ├── UiMenu.h/cpp          # Menü-System (Terrain, Aktionen)
│       └── DisplayOLED.h/cpp     # OLED Status-Anzeige
└── platformio.ini                # PlatformIO-Konfiguration
```

## 🔧 Hardware

### Robot (ESP8266)

#### Servo-Belegung (von oben gesehen)

| Bein | Position | Servo | GPIO | Funktion |
|------|----------|-------|------|----------|
| Vorne-Rechts | UR | 0 | G14 | Klaue (PAW) |
| Vorne-Rechts | UR | 1 | G12 | Schulter (ARM) |
| Hinten-Rechts | LR | 2 | G13 | Schulter (ARM) |
| Hinten-Rechts | LR | 3 | G15 | Klaue (PAW) |
| Vorne-Links | UL | 4 | G16 | Klaue (PAW) |
| Vorne-Links | UL | 5 | G5 | Schulter (ARM) |
| Hinten-Links | LL | 6 | G4 | Schulter (ARM) |
| Hinten-Links | LL | 7 | G2 | Klaue (PAW) |

### Remote Controller (ESP32)

| Komponente | Pin(s) | Beschreibung |
|------------|--------|--------------|
| Joystick X | GPIO 34 | Analog (ADC1) |
| Joystick Y | GPIO 35 | Analog (ADC1) |
| Poti Speed | GPIO 32 | Maximale Geschwindigkeit |
| Poti Turn  | GPIO 33 | Dreh-Steuerung |
| Button L   | GPIO 25 | Menü links |
| Button M   | GPIO 26 | Menü auswählen (lang: Modus wechseln) |
| Button R   | GPIO 27 | Menü rechts |
| Button STOP| GPIO 14 | Not-Stopp |
| OLED SDA   | GPIO 21 | I2C Display |
| OLED SCL   | GPIO 22 | I2C Display |

## 🚀 Installation

### Voraussetzungen

- [PlatformIO](https://platformio.org/) (VS Code Extension oder CLI)
- ESP8266 NodeMCU Board (Robot)
- ESP32 DevKit Board (Remote Controller)

### Robot (ESP8266)

```bash
cd Spider

# Firmware kompilieren und hochladen
pio run --target upload

# Web-Dateien auf LittleFS hochladen
pio run --target uploadfs
```

### Remote Controller (ESP32)

```bash
cd SpiderRemote-ESP32

# Firmware kompilieren und hochladen
pio run --target upload
```

### WLAN-Konfiguration

**Robot** (`src/main.cpp`):
```cpp
const char* HOME_SSID = "DeinWLAN";
const char* HOME_PASSWORD = "DeinPasswort";
```

**Remote** (`SpiderRemote-ESP32/src/Config.h`):
```cpp
// Robot AP-Modus
static const char* WIFI_AP_SSID   = "QuadBot-E";
static const char* WIFI_AP_PASS   = "123456";
static const char* SPIDER_AP_HOST = "192.168.4.1";

// Fallback: Heimnetzwerk
static const char* WIFI_HOME_SSID   = "DeinWLAN";
static const char* WIFI_HOME_PASS   = "DeinPasswort";
static const char* SPIDER_HOME_HOST = "10.0.0.11";
```

## 🎮 Bedienung

### Web-Interface

1. ESP8266 mit Strom versorgen
2. Im WLAN verbinden (oder AP-Modus: "QuadBot-E")
3. Browser öffnen: `http://<IP-Adresse>`

- **D-Pad**: Kontinuierliche Bewegungssteuerung (gedrückt halten)
- **Aktionen**: Einmaliges Ausführen von Bewegungsabläufen
- **Terrain**: Anpassung für Steigungen
- **Geschwindigkeit**: Bewegungstempo regulieren
- **Kalibrierung**: Servo-Offsets einstellen und speichern

### Remote Controller

1. Remote einschalten → verbindet automatisch (AP oder Heimnetz)
2. OLED zeigt: Verbindungsstatus, Modus, aktuelle Bewegung

**DRIVE Modus:**
- Joystick: Bewegungsrichtung
- Speed-Poti: Maximale Geschwindigkeit
- Turn-Poti: Drehen
- STOP-Button: Sofort anhalten

**TERRAIN Modus** (MID lang drücken):
- L/R: Terrain wählen (Flach/Bergauf/Bergab)
- MID: Bestätigen

**ACTION Modus** (MID nochmal lang drücken):
- L/R: Aktion wählen
- MID: Ausführen

### Serial Debug (ohne Hardware)

In `Config.h`:
```cpp
#define SERIAL_CMD_MODE true
```

Dann im Serial Monitor (115200 baud):
```
f        → forward
b        → backward
hello    → Wave-Animation
dance1   → Tanz
status   → Zeigt WiFi/WS Status
help     → Alle Befehle
```

## 📡 WebSocket-Protokoll

| Typ | Befehl | Beschreibung |
|-----|--------|--------------|
| `moveStart` | forward, backward, left, right, turnleft, turnright | Kontinuierliche Bewegung |
| `moveStop` | - | Bewegung stoppen (nach Sequenz) |
| `stop` | - | Sofort stoppen |
| `cmd` | standby, sleep, lie, hello, pushup, fighting, dance1-3 | Einzelaktion |
| `setSpeed` | speed: 10-100 | Geschwindigkeit |
| `setTerrain` | mode: normal/uphill/downhill | Terrain-Modus |
| `setOffsets` | offsets: [8 Werte] | Kalibrierung |
| `getOffsets` | - | Offsets abrufen |

## 🔒 Robustheit

- **WiFi Auto-Fallback**: Remote versucht AP, dann Heimnetz
- **WiFi Reconnect**: Automatische Wiederverbindung bei Verlust
- **WebSocket Reconnect**: Automatisch alle 2 Sekunden
- **Rate Limiting**: Max 20 Commands/Sekunde (50ms Intervall)
- **Not-Stopp**: STOP-Button hat höchste Priorität

## 📜 Lizenz

MIT License
