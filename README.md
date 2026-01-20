# 🕷️ Spider Robot

Ein vierbeiniger Spinnenroboter mit ESP8266 und 8 Servos, gesteuert über eine Web-Oberfläche.

![ESP8266](https://img.shields.io/badge/ESP8266-NodeMCU-blue)
![PlatformIO](https://img.shields.io/badge/PlatformIO-Arduino-orange)
![License](https://img.shields.io/badge/License-MIT-green)

## 🎯 Features

- **8 Servos** für 4 Beine (Schulter + Klaue pro Bein)
- **Web-Interface** mit Echtzeit-Steuerung via WebSocket
- **Bewegungen**: Vorwärts, Rückwärts, Seitwärts, Drehungen
- **Aktionen**: Stehen, Schlafen, Winken, Liegestütz, Tanzen, Kämpfen
- **Terrain-Modus**: Flach, Bergauf, Bergab
- **Kalibrierung**: Servo-Offsets über UI einstellbar und persistent gespeichert
- **Responsive Design**: Funktioniert auf Desktop und Mobilgeräten

## 🔧 Hardware

### Servo-Belegung (von oben gesehen)

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

## 📁 Projektstruktur

```
Spider/
├── src/
│   ├── main.cpp              # Hauptprogramm
│   ├── motion/
│   │   ├── MotionData.h/cpp  # Bewegungsmatrizen & Servo-Steuerung
│   ├── robot/
│   │   ├── RobotController.h/cpp  # Roboter-Logik & Befehlsverarbeitung
│   ├── web/
│   │   ├── WebServer.h/cpp   # WebSocket & HTTP-Server
│   └── wifi/
│       └── WifiManager.h/cpp # WLAN-Verbindung & AP-Modus
├── data/
│   └── index.html            # Web-Interface
└── platformio.ini            # PlatformIO-Konfiguration
```

## 🚀 Installation

### Voraussetzungen

- [PlatformIO](https://platformio.org/) (VS Code Extension oder CLI)
- ESP8266 NodeMCU Board

### Build & Upload

```bash
# Firmware kompilieren und hochladen
pio run --target upload

# Web-Dateien auf LittleFS hochladen
pio run --target uploadfs
```

### WLAN-Konfiguration

In `src/main.cpp` die WLAN-Zugangsdaten eintragen:

```cpp
const char* HOME_SSID = "DeinWLAN";
const char* HOME_PASSWORD = "DeinPasswort";
```

## 🎮 Bedienung

1. ESP8266 mit Strom versorgen
2. Im WLAN verbinden (oder AP-Modus: "Spider-AP")
3. Browser öffnen: `http://<IP-Adresse>`

### Web-Interface

- **D-Pad**: Kontinuierliche Bewegungssteuerung (gedrückt halten)
- **Aktionen**: Einmaliges Ausführen von Bewegungsabläufen
- **Terrain**: Anpassung für Steigungen
- **Geschwindigkeit**: Bewegungstempo regulieren
- **Kalibrierung**: Servo-Offsets einstellen und speichern

## 📡 WebSocket-Befehle

| Typ | Befehl | Beschreibung |
|-----|--------|--------------|
| `moveStart` | forward, backward, left, right, turnLeft, turnRight | Kontinuierliche Bewegung starten |
| `moveStop` | - | Bewegung stoppen |
| `cmd` | standby, sleep, lie, hello, pushup, fighting, dance1-3 | Einzelaktion ausführen |
| `setOffsets` | offsets: [8 Werte] | Kalibrierung setzen & speichern |
| `getOffsets` | - | Aktuelle Offsets abrufen |
| `setSpeed` | speed: 10-100 | Geschwindigkeit setzen |
| `setTerrain` | mode: normal/uphill/downhill | Terrain-Modus setzen |

## 📜 Lizenz

MIT License
