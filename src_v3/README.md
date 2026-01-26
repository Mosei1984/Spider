# Spider Controller v3 - Gait Runtime Optimierung

## Übersicht

v3 implementiert erweiterte Laufzeit-Optimierungen für den Spider Controller:

### Features

| Feature | Beschreibung |
|---------|--------------|
| **Stride-Skalierung** | Hip-Servos stärker, Knee-Servos moderater skalieren |
| **Micro-Stepping** | Feinere Interpolation zwischen Keyframes (1-16 Substeps) |
| **Timing-Shaping** | Swing-Phase schneller, Stance-Phase langsamer |
| **Soft-Start/Stop** | Sanftes An- und Ausrampen des Stride-Faktors |
| **Servo-Kalibrierung** | Min/Max/Center Limits pro Servo |

---

## Ordnerstruktur

```
src_v3/
├── main_v3.cpp           # Hauptdatei
├── gait/
│   ├── GaitConfig.h      # Konfigurationsstrukturen
│   ├── GaitRuntime.h     # API Header
│   └── GaitRuntime.cpp   # Motion Engine
├── motion/
│   ├── MotionData_v3.h   # Servo-Definitionen
│   └── MotionData_v3.cpp # PROGMEM Keyframes + Low-Level
├── robot/
│   ├── RobotController_v3.h
│   └── RobotController_v3.cpp
├── calibration/
│   ├── ServoCalibration.h
│   └── ServoCalibration.cpp
└── web/
    ├── WebServer_v3.h
    └── WebServer_v3.cpp

SpiderRemote-ESP32/src/v3/
├── WalkParams.h          # Parameter-Definitionen
├── WsClientV3.h/.cpp     # Erweiterter WebSocket Client
├── DriveControlV3.h/.cpp # Erweiterter Drive Controller
└── UiMenuV3.h/.cpp       # Erweitertes UI-Menü
```

---

## Integration

### 1. ESP8266 Spider Controller

**Option A: Kompletter v3-Wechsel**
```ini
# platformio.ini
[env:spider_v3]
platform = espressif8266
board = d1_mini
framework = arduino
src_dir = src_v3
```

**Option B: Koexistenz mit v2**
- Beide Versionen bleiben erhalten
- Umschalten durch Ändern von `src_dir`

### 2. ESP32 SpiderRemote

In `main.cpp`:
```cpp
#include "v3/WsClientV3.h"
#include "v3/DriveControlV3.h"
#include "v3/UiMenuV3.h"

WsClientV3 wsClient;
DriveControlV3 driveControl;
UiMenuV3 uiMenu;
```

---

## WebSocket API v3

### Walk-Parameter Commands

```json
// Alle Parameter setzen
{
  "type": "setWalkParams",
  "stride": 1.0,
  "subSteps": 8,
  "profile": 1,
  "swingMul": 0.75,
  "stanceMul": 1.25,
  "rampEnabled": false,
  "rampCycles": 3
}

// Einzelne Parameter
{"type": "setStride", "value": 1.2}
{"type": "setSubSteps", "value": 12}
{"type": "setTimingProfile", "value": 1}
{"type": "setSwingMul", "value": 0.8}
{"type": "setStanceMul", "value": 1.3}
{"type": "setRamp", "enabled": true, "cycles": 5}

// moveStart mit Override
{
  "type": "moveStart",
  "name": "forward",
  "stride": 1.5,
  "subSteps": 16
}
```

### Servo-Kalibrierung

```json
// Offset setzen
{"type": "setServoOffset", "servo": 0, "offset": 5}

// Limits setzen
{"type": "setServoLimits", "servo": 0, "min": 30, "max": 150, "center": 90}

// Komplett
{"type": "setServoCalib", "servo": 0, "offset": 5, "min": 30, "max": 150, "center": 90}

// Speichern/Laden
{"type": "saveCalib"}
{"type": "loadCalib"}
```

---

## Parameter-Erklärung

### Stride-Skalierung

| Parameter | Bereich | Default | Beschreibung |
|-----------|---------|---------|--------------|
| `stride` | 0.3-2.0 | 1.0 | Gesamtskalierung der Schrittweite |
| `kneeMix` | 0.0-1.0 | 0.5 | Wie stark Knee-Servos mitskalieren |

**Formel:**
- Hip: `angle = center + (raw - center) * stride`
- Knee: `angle = center + (raw - center) * (1 + kneeMix * (stride - 1))`

### Timing-Shaping

| Parameter | Bereich | Default | Beschreibung |
|-----------|---------|---------|--------------|
| `profile` | 0-2 | 1 | 0=Linear, 1=SwingStance, 2=EaseInOut |
| `swingMul` | 0.3-2.0 | 0.75 | Multiplikator für Swing-Phase (Bein hebt) |
| `stanceMul` | 0.3-2.0 | 1.25 | Multiplikator für Stance-Phase (Bein am Boden) |
| `liftThreshold` | 1-30 | 8 | Mindest-Delta für Swing-Erkennung (Grad) |

### Interpolation

| Parameter | Bereich | Default | Beschreibung |
|-----------|---------|---------|--------------|
| `subSteps` | 1-16 | 8 | Substeps pro Keyframe-Übergang |
| `smoothstepEnabled` | bool | true | Smoothstep Easing aktiviert |

### Soft-Ramp

| Parameter | Bereich | Default | Beschreibung |
|-----------|---------|---------|--------------|
| `rampEnabled` | bool | false | Soft-Start aktiviert |
| `rampCycles` | 1-10 | 3 | Anzahl Zyklen für Ramp |

---

## Servo-Mapping

| Index | GPIO | Position | Typ |
|-------|------|----------|-----|
| 0 | 14 | UR paw | Knee |
| 1 | 12 | UR arm | Hip |
| 2 | 13 | LR arm | Hip |
| 3 | 15 | LR paw | Knee |
| 4 | 16 | UL paw | Knee |
| 5 | 5 | UL arm | Hip |
| 6 | 4 | LL arm | Hip |
| 7 | 2 | LL paw | Knee |

---

## Architektur-Diagramm

```
┌─────────────────────────────────────────────────────────────┐
│                    SpiderRemote-ESP32                       │
│  ┌───────────┐  ┌──────────────┐  ┌──────────────────────┐ │
│  │ UiMenuV3  │──│DriveControlV3│──│    WsClientV3        │ │
│  └───────────┘  └──────────────┘  └──────────────────────┘ │
│                         │                    │              │
└─────────────────────────┼────────────────────┼──────────────┘
                          │  WebSocket JSON    │
                          ▼                    ▼
┌─────────────────────────────────────────────────────────────┐
│                    ESP8266 Spider                            │
│  ┌───────────────┐  ┌─────────────────┐  ┌───────────────┐  │
│  │ WebServer_v3  │──│RobotController_v3│──│ GaitRuntime   │  │
│  └───────────────┘  └─────────────────┘  └───────────────┘  │
│                              │                   │          │
│                    ┌─────────┴─────────┐         │          │
│                    ▼                   ▼         ▼          │
│             ┌────────────┐    ┌──────────────────────────┐  │
│             │ServoCalib  │────│      MotionData_v3       │  │
│             └────────────┘    │  (PROGMEM Keyframes)     │  │
│                               └──────────────────────────┘  │
│                                          │                  │
│                                          ▼                  │
│                               ┌──────────────────────────┐  │
│                               │    8x Servo (PWM)        │  │
│                               └──────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

---

## Datei-Änderungen für Migration

### Wenn von src/ zu src_v3/:

1. **WiFi-Credentials in main_v3.cpp anpassen**
2. **platformio.ini: `src_dir = src_v3`**
3. **Bestehende Offsets werden migriert** (anderes Dateiformat)

### Bestehende Funktionalität bleibt:

- Alle Original-Commands funktionieren
- Terrain-Blending unverändert
- Dance/Hello/Pushup blockierend (Legacy)
- Walk-Commands (Forward/Back/etc.) nutzen GaitRuntime

---

## Bekannte Einschränkungen

1. **Float-Performance:** ESP8266 hat keine FPU. Bei extremen SubSteps (>12) kann Jitter auftreten.
2. **PROGMEM:** Keyframes werden nicht modifiziert. Stride-Skalierung erfolgt zur Laufzeit.
3. **Blocking Legacy:** Dance/Hello etc. nutzen weiterhin blockierenden Code.

---

## Nächste Schritte (optional)

- [ ] Fixed-Point (Q16) für Interpolation bei Performance-Problemen
- [ ] Per-Leg Phase Engine für komplexere Gaits
- [ ] Dynamic Keyframe Generation
- [ ] OTA-Updates für Remote
