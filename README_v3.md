# Spider Robot v3 - Complete System

Quadruped robot with ESP8266 controller and ESP32 handheld remote.

## Overview

v3 is a complete rewrite with enhanced gait control and a full-featured remote UI.

| Component | MCU | Description |
|-----------|-----|-------------|
| **Spider Bot** | ESP8266 (NodeMCU) | 8-servo quadruped with WebSocket API |
| **SpiderRemote** | ESP32 | OLED display, joystick, buttons, MPU6050 |

---

## Features

### Spider Bot (ESP8266)

| Feature | Description |
|---------|-------------|
| **Stride Scaling** | Adjustable step width (0.3x - 2.0x) |
| **Micro-Stepping** | 1-16 substeps per keyframe for smooth motion |
| **Timing Shaping** | Swing phase faster, stance phase slower |
| **Soft Ramp** | Gradual start/stop over configurable cycles |
| **Servo Calibration** | Per-servo offset, min, max, center angles |
| **Terrain Modes** | Normal, uphill, downhill with body tilt |
| **WiFi AP Mode** | Creates `QuadBot-E` network (fallback) |

### SpiderRemote (ESP32)

| Feature | Description |
|---------|-------------|
| **Drive Mode** | Joystick control (forward/back, strafe, turn) |
| **Motion Mode** | MPU6050 tilt control with calibration |
| **Walk Parameters** | Stride, SubSteps, Profile, SwingMul, StanceMul, Ramp |
| **Servo Calibration** | Edit offsets/limits, sync with bot, save locally |
| **Terrain Switching** | Direct LEFT/RIGHT buttons in Drive/Motion mode |
| **Actions** | Hello, dance, pushup, standby, etc. |
| **NVS Persistence** | All calibrations saved in flash |

---

## Quick Start

### 1. Flash Spider Bot

```bash
# Build and upload ESP8266 firmware
pio run -e spider_v3 -t upload

# Upload web interface (optional)
pio run -e spider_v3 -t uploadfs
```

### 2. Flash SpiderRemote

Edit `platformio.ini` and set your WiFi credentials:
```ini
-DWIFI_SSID=\"YOUR_SSID\"
-DWIFI_PASS=\"YOUR_PASSWORD\"
-DSPIDER_HOST=\"192.168.4.1\"
```

```bash
# Build and upload ESP32 firmware
pio run -e esp32remote_v3 -t upload
```

### 3. Connect

1. Power on Spider Bot → Creates WiFi `QuadBot-E` (password: `12345678`)
2. Power on Remote → Connects automatically
3. Use joystick to control movement

---

## Remote Control UI

### Menu Modes (Long-press MID button to cycle)

| Mode | Description |
|------|-------------|
| **DRIVE** | Joystick control, LEFT/RIGHT = terrain |
| **MOTION** | MPU6050 tilt control |
| **INPUT_CALIB** | Joystick/potentiometer calibration |
| **WALK_PARAMS** | Adjust gait parameters |
| **SERVO_CALIB** | Servo offset and limits |
| **ACTIONS** | Execute predefined actions |
| **TERRAIN** | Select terrain mode |

### Button Layout

```
[LEFT]     [MID]      [RIGHT]
  ↓          ↓           ↓
Decrease   Select    Increase
/ Prev     / OK      / Next
```

### DRIVE Mode Display
```
== DRIVE ==
Joystick aktiv
[<] normal [>]
[HOLD MID] Menu
```

### SERVO_CALIB Menu Options

| # | Option | Function |
|---|--------|----------|
| 0 | Servo bearbeiten | Select and edit servo |
| 1 | Grundstellung | All servos to 90° |
| 2 | Entsperren/Sperren | Toggle calibration lock |
| 3 | Vom Bot laden | Request calibration from bot |
| 4 | Lokal speichern | Save to NVS |
| 5 | An Bot senden | Send all to bot + save |

---

## WebSocket API

### Movement Commands

```json
{"type": "moveStart", "name": "forward"}
{"type": "moveStart", "name": "backward"}
{"type": "moveStart", "name": "left"}
{"type": "moveStart", "name": "right"}
{"type": "moveStart", "name": "turnleft"}
{"type": "moveStart", "name": "turnright"}
{"type": "moveStop"}
{"type": "stop"}
```

### Walk Parameters

```json
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
```

### Servo Calibration

```json
{"type": "setServoCalib", "servo": 0, "offset": 5, "min": 30, "max": 150, "center": 90}
{"type": "getServoCalib"}
{"type": "saveCalib"}
{"type": "setCalibLock", "locked": false}
```

### Terrain

```json
{"type": "setTerrain", "mode": "normal"}
{"type": "setTerrain", "mode": "uphill"}
{"type": "setTerrain", "mode": "downhill"}
```

---

## Hardware

### Spider Bot - Servo Mapping

| Index | GPIO | Position | Type |
|-------|------|----------|------|
| 0 | 14 | UR paw | Knee |
| 1 | 12 | UR arm | Hip |
| 2 | 13 | LR arm | Hip |
| 3 | 15 | LR paw | Knee |
| 4 | 16 | UL paw | Knee |
| 5 | 5 | UL arm | Hip |
| 6 | 4 | LL arm | Hip |
| 7 | 2 | LL paw | Knee |

### SpiderRemote - Pin Configuration

| Component | Pins |
|-----------|------|
| OLED (I2C) | SDA=21, SCL=22 |
| Joystick | X=34, Y=35 |
| Speed Poti | 32 |
| Turn Poti | 33 |
| Buttons | LEFT=27, MID=14, RIGHT=12, STOP=13 |
| MPU6050 | I2C (same bus as OLED) |

---

## Project Structure

```
Spider/
├── src_v3/                      # ESP8266 Spider Controller v3
│   ├── main_v3.cpp
│   ├── gait/
│   │   ├── GaitConfig.h
│   │   ├── GaitRuntime.h/.cpp
│   ├── motion/
│   │   ├── MotionData_v3.h/.cpp
│   ├── robot/
│   │   ├── RobotController_v3.h/.cpp
│   ├── calibration/
│   │   ├── ServoCalibration.h/.cpp
│   └── web/
│       ├── WebServer_v3.h/.cpp
│
├── SpiderRemote-ESP32/src/v3/   # ESP32 Remote Controller v3
│   ├── main_v3.cpp
│   ├── WalkParams.h
│   ├── WsClientV3.h/.cpp
│   ├── DriveControlV3.h/.cpp
│   ├── UiMenuV3.h/.cpp
│   └── ServoCalibStore.h/.cpp
│
├── data_v3/                     # Web interface files
│   └── index.html
│
└── platformio.ini               # Build configuration
```

---

## PlatformIO Environments

| Environment | Target | Description |
|-------------|--------|-------------|
| `spider_v3` | ESP8266 | Spider bot firmware |
| `esp32remote_v3` | ESP32 | Remote controller firmware |

---

## Troubleshooting

### Remote doesn't connect
- Check WiFi credentials in `platformio.ini`
- Verify Spider Bot is powered and creating AP
- Default AP: `QuadBot-E`, Password: `12345678`

### Servos don't move
- Check calibration lock status (unlock via menu or WebSocket)
- Verify servo power supply (5V, adequate current)
- Use "Grundstellung" to test all servos at 90°

### Jerky movement
- Reduce `subSteps` (8 is recommended for ESP8266)
- Check for WiFi interference
- Verify adequate power supply

---

## License

MIT License - See LICENSE file for details.
