#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>

#include "Config.h"
#include "WsClient.h"
#include "Inputs.h"
#include "Buttons.h"
#include "DriveControl.h"
#include "UiMenu.h"
#include "DisplayOLED.h"

static WsClient ws;
static Inputs inputs;
static DriveControl drive;

static Button btnLeft, btnMid, btnRight, btnStop;

static UiMenu uiMenu;
static UiState uiState;

static DisplayOLED oled;

static uint32_t lastWifiCheck = 0;
static bool wasConnected = false;

// Aktive Verbindung
static const char* activeSSID = nullptr;
static const char* activePass = nullptr;
static const char* activeHost = nullptr;
static bool usingAPMode = true;

#if SERIAL_CMD_MODE
static char serialBuf[64];
static int serialIdx = 0;

static void printHelp() {
  Serial.println("\n=== Serial Command Mode ===");
  Serial.println("Movement:  forward, backward, left, right, turnleft, turnright");
  Serial.println("Control:   stop, movestop");
  Serial.println("Speed:     speed <0-100>  (e.g. 'speed 50')");
  Serial.println("Terrain:   terrain <normal|uphill|downhill>");
  Serial.println("Actions:   hello, dance1, dance2, dance3, standby, sleep, lie, pushup, fighting");
  Serial.println("Status:    status");
  Serial.println("Help:      help, ?");
  Serial.println("============================\n");
}

static void handleSerialCommand(const char* cmd) {
  String c = String(cmd);
  c.trim();
  c.toLowerCase();
  
  if (c.length() == 0) return;
  
  Serial.printf("[CMD] '%s'\n", c.c_str());
  
  // Movement commands (send speed + move without rate limiting)
  if (c == "forward" || c == "f") {
    ws.sendImmediate("{\"type\":\"setSpeed\",\"speed\":50}");
    ws.sendImmediate("{\"type\":\"moveStart\",\"name\":\"forward\"}");
  } else if (c == "backward" || c == "b" || c == "back") {
    ws.sendImmediate("{\"type\":\"setSpeed\",\"speed\":50}");
    ws.sendImmediate("{\"type\":\"moveStart\",\"name\":\"backward\"}");
  } else if (c == "left" || c == "l") {
    ws.sendImmediate("{\"type\":\"setSpeed\",\"speed\":50}");
    ws.sendImmediate("{\"type\":\"moveStart\",\"name\":\"left\"}");
  } else if (c == "right" || c == "r") {
    ws.sendImmediate("{\"type\":\"setSpeed\",\"speed\":50}");
    ws.sendImmediate("{\"type\":\"moveStart\",\"name\":\"right\"}");
  } else if (c == "turnleft" || c == "tl") {
    ws.sendImmediate("{\"type\":\"setSpeed\",\"speed\":50}");
    ws.sendImmediate("{\"type\":\"moveStart\",\"name\":\"turnleft\"}");
  } else if (c == "turnright" || c == "tr") {
    ws.sendImmediate("{\"type\":\"setSpeed\",\"speed\":50}");
    ws.sendImmediate("{\"type\":\"moveStart\",\"name\":\"turnright\"}");
  }
  // Stop commands
  else if (c == "stop" || c == "s") {
    ws.sendStop();
  } else if (c == "movestop" || c == "ms") {
    ws.sendMoveStop();
  }
  // Speed command
  else if (c.startsWith("speed ")) {
    int spd = c.substring(6).toInt();
    ws.sendSetSpeed(spd);
    Serial.printf("[CMD] Speed set to %d\n", spd);
  }
  // Terrain command
  else if (c.startsWith("terrain ")) {
    String mode = c.substring(8);
    ws.sendTerrain(mode.c_str());
  } else if (c == "normal" || c == "uphill" || c == "downhill") {
    ws.sendTerrain(c.c_str());
  }
  // Action commands
  else if (c == "hello" || c == "dance1" || c == "dance2" || c == "dance3" ||
           c == "standby" || c == "sleep" || c == "lie" || c == "pushup" || c == "fighting") {
    ws.sendCmd(c.c_str());
  }
  // Status
  else if (c == "status" || c == "st") {
    Serial.println("\n=== STATUS ===");
    Serial.printf("WiFi: %s (%s)\n", 
      WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected",
      activeSSID ? activeSSID : "none");
    Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("WebSocket: %s (Host: %s)\n", 
      ws.connected() ? "Connected" : "Disconnected",
      activeHost ? activeHost : "none");
    Serial.printf("Mode: %s\n", usingAPMode ? "AP" : "Home Network");
    Serial.println("==============\n");
  }
  // Help
  else if (c == "help" || c == "?" || c == "h") {
    printHelp();
  }
  else {
    Serial.printf("[CMD] Unknown: '%s' (type 'help')\n", c.c_str());
  }
}

static void processSerial() {
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') {
      if (serialIdx > 0) {
        serialBuf[serialIdx] = '\0';
        handleSerialCommand(serialBuf);
        serialIdx = 0;
      }
    } else if (serialIdx < (int)sizeof(serialBuf) - 1) {
      serialBuf[serialIdx++] = ch;
    }
  }
}
#endif

static bool tryConnect(const char* ssid, const char* pass, uint32_t timeoutMs) {
  Serial.printf("[WiFi] Trying %s...\n", ssid);
  
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > timeoutMs) {
      Serial.printf("[WiFi] Timeout connecting to %s\n", ssid);
      return false;
    }
    delay(100);
  }
  Serial.printf("[WiFi] Connected to %s (IP: %s)\n", ssid, WiFi.localIP().toString().c_str());
  return true;
}

static void connectWifiWithFallback() {
  // Erst AP-Modus versuchen
  if (tryConnect(WIFI_AP_SSID, WIFI_AP_PASS, WIFI_AP_TIMEOUT_MS)) {
    activeSSID = WIFI_AP_SSID;
    activePass = WIFI_AP_PASS;
    activeHost = SPIDER_AP_HOST;
    usingAPMode = true;
    return;
  }
  
  // Fallback: Heimnetzwerk
  if (tryConnect(WIFI_HOME_SSID, WIFI_HOME_PASS, WIFI_HOME_TIMEOUT_MS)) {
    activeSSID = WIFI_HOME_SSID;
    activePass = WIFI_HOME_PASS;
    activeHost = SPIDER_HOME_HOST;
    usingAPMode = false;
    return;
  }
  
  // Beide fehlgeschlagen - defaults setzen für Retry im loop
  Serial.println("[WiFi] All connections failed, will retry...");
  activeSSID = WIFI_AP_SSID;
  activePass = WIFI_AP_PASS;
  activeHost = SPIDER_AP_HOST;
  usingAPMode = true;
}

void setup() {
  Serial.begin(115200);

  Wire.begin(OLED_SDA, OLED_SCL);

  btnLeft.begin(PIN_BTN_LEFT, BTN_DEBOUNCE_MS, BTN_LONGPRESS_MS);
  btnMid.begin(PIN_BTN_MID, BTN_DEBOUNCE_MS, BTN_LONGPRESS_MS);
  btnRight.begin(PIN_BTN_RIGHT, BTN_DEBOUNCE_MS, BTN_LONGPRESS_MS);
  btnStop.begin(PIN_BTN_STOP, BTN_DEBOUNCE_MS, BTN_LONGPRESS_MS);

  oled.begin(OLED_UPDATE_MS);

  connectWifiWithFallback();
  ws.begin(activeHost, SPIDER_PORT, SPIDER_PATH, MIN_SEND_INTERVAL_MS);

  inputs.begin(PIN_JOY_X, PIN_JOY_Y, PIN_POT_VMAX, PIN_POT_TURN, ADC_MAX);

  drive.begin(&ws);
  uiMenu.begin(&ws);

  uiState.mode = UiMode::DRIVE;
  uiState.terrainIndex = 0;
  uiState.actionIndex = 3;
  uiState.terrainActive = "normal";

#if SERIAL_CMD_MODE
  printHelp();
#endif
}

void loop() {
  uint32_t now = millis();
  
  if (now - lastWifiCheck > WIFI_RECONNECT_INTERVAL_MS) {
    lastWifiCheck = now;
    bool isConnected = (WiFi.status() == WL_CONNECTED);
    
    if (!isConnected) {
      Serial.println("[WiFi] Lost connection, reconnecting...");
      // Bei Reconnect: mit Fallback versuchen
      connectWifiWithFallback();
      ws.begin(activeHost, SPIDER_PORT, SPIDER_PATH, MIN_SEND_INTERVAL_MS);
    } else if (!wasConnected && isConnected) {
      Serial.println("[WiFi] Reconnected, restarting WebSocket");
      ws.reconnect();
    }
    wasConnected = isConnected;
  }

  ws.loop();

#if SERIAL_CMD_MODE
  processSerial();
#endif

  btnLeft.update();
  btnMid.update();
  btnRight.update();
  btnStop.update();

  PressType pStop = btnStop.consume();
  if (pStop == PressType::Short || pStop == PressType::Long) {
    drive.forceStop();
  }

  uiMenu.tick(btnLeft, btnMid, btnRight, uiState);

#if SERIAL_CMD_MODE
  // Skip joystick input when testing via Serial (no hardware)
  InputState in = {SPEED_MIN, 0, 0, 0};
#else
  InputState in = inputs.read(DEAD_JOY, DEAD_TURN, SPEED_MIN, SPEED_MAX);
  drive.tick(in, SPEED_MIN);
#endif

  oled.tick(ws.connected(), in.speedMax, drive.currentSpeed(), drive.currentMove(), uiState);

}
