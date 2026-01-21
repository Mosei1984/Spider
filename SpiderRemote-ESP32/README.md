# SpiderRemote-ESP32

Modulare ESP32-Fernbedienung fuer Mosei1984/Spider (WebSocket-kompatibel mit dem Web-UI).

## Features
- Joystick: Vor/Zurueck/Links/Rechts (proportional ueber setSpeed)
- Poti 1: Max. Geschwindigkeit (10..100)
- Poti 2: Drehen (turnLeft/turnRight proportional)
- OLED 128x64 SSD1306 (I2C)
- 3-Button-Menue: Links/Rechts waehlen, Mitte kurz bestaetigen, Mitte lang Mode wechseln
- Separater STOP-Button (Failsafe)

## WebSocket
- Endpoint: ws://<spider-ip>/ws
- JSON wie im index.html:
  - {"type":"moveStart","name":"forward|backward|left|right|turnLeft|turnRight"}
  - {"type":"moveStop"}
  - {"type":"stop"}
  - {"type":"setSpeed","speed":10..100}
  - {"type":"setTerrain","mode":"normal|uphill|downhill"}
  - {"type":"cmd","name":"hello|pushup|dance1|..."}

## Build (PlatformIO)
- SSID/PASS & Spider-IP in src/Config.h setzen
- Build: pio run
- Upload: pio run -t upload
