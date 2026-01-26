// =============================================================================
// main_v3.cpp - Eigenständige SpiderRemote v3 Hauptdatei
// =============================================================================
// ESP32 SpiderRemote Controller v3
// Features:
//   - Walk-Parameter Steuerung (Stride, SubSteps, Timing)
//   - Servo-Kalibrierung (Offset, Limits pro Servo)
//   - Erweiterte UI-Menüs
// =============================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>

// Basis-Komponenten aus Haupt-src (via Include-Pfad)
#include "Config.h"
#include "Inputs.h"
#include "Buttons.h"
#include "DisplayOLED.h"
#include "MotionInputs.h"
#include "InputCalib.h"

// v3-Komponenten
#include "WalkParams.h"
#include "WsClientV3.h"
#include "DriveControlV3.h"
#include "UiMenuV3.h"

// =============================================================================
// Globale Instanzen
// =============================================================================
static WsClientV3 ws;
static Inputs inputs;
static MotionInputs motion;
static InputCalib inputCalib;
static DriveControlV3 drive;

static Button btnLeft, btnMid, btnRight, btnStop;
static UiMenuV3 uiMenu;
static DisplayOLED oled;

// Verbindungsstatus
static uint32_t lastWifiCheck = 0;
static bool wasConnected = false;
static const char* activeSSID = nullptr;
static const char* activePass = nullptr;
static const char* activeHost = nullptr;
static bool usingAPMode = true;

// =============================================================================
// WiFi-Verbindung
// =============================================================================
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
    // Erst Heimnetzwerk versuchen (Spider verbindet sich auch zuerst hierhin)
    if (tryConnect(WIFI_HOME_SSID, WIFI_HOME_PASS, WIFI_HOME_TIMEOUT_MS)) {
        activeSSID = WIFI_HOME_SSID;
        activePass = WIFI_HOME_PASS;
        activeHost = SPIDER_HOME_HOST;
        usingAPMode = false;
        return;
    }
    
    // Fallback: Spider AP-Modus (wenn Spider kein Heimnetzwerk gefunden hat)
    if (tryConnect(WIFI_AP_SSID, WIFI_AP_PASS, WIFI_AP_TIMEOUT_MS)) {
        activeSSID = WIFI_AP_SSID;
        activePass = WIFI_AP_PASS;
        activeHost = SPIDER_AP_HOST;
        usingAPMode = true;
        return;
    }
    
    Serial.println("[WiFi] All connections failed, will retry...");
    activeSSID = WIFI_HOME_SSID;
    activePass = WIFI_HOME_PASS;
    activeHost = SPIDER_HOME_HOST;
    usingAPMode = false;
}

// =============================================================================
// Serial Command Handler (optional)
// =============================================================================
#if SERIAL_CMD_MODE
static char serialBuf[64];
static int serialIdx = 0;

static void printHelp() {
    Serial.println(F("\n=== Spider Remote v3 - Serial Commands ==="));
    Serial.println(F("Movement:  forward, backward, left, right, turnleft, turnright"));
    Serial.println(F("Control:   stop, movestop"));
    Serial.println(F("Speed:     speed <0-100>"));
    Serial.println(F("Walk v3:   stride <0.3-2.0>, substeps <1-16>"));
    Serial.println(F("Calib:     offset <servo> <value>, limits <servo> <min> <max> <center>"));
    Serial.println(F("Actions:   hello, dance1, standby, sleep"));
    Serial.println(F("Status:    status"));
    Serial.println(F("==========================================\n"));
}

static void handleSerialCommand(const char* cmd) {
    String c = String(cmd);
    c.trim();
    c.toLowerCase();
    
    if (c.length() == 0) return;
    
    Serial.printf("[CMD] '%s'\n", c.c_str());
    
    // Movement commands
    if (c == "forward" || c == "f") {
        ws.sendMoveStart("forward");
    } else if (c == "backward" || c == "b") {
        ws.sendMoveStart("backward");
    } else if (c == "left" || c == "l") {
        ws.sendMoveStart("left");
    } else if (c == "right" || c == "r") {
        ws.sendMoveStart("right");
    } else if (c == "turnleft" || c == "tl") {
        ws.sendMoveStart("turnleft");
    } else if (c == "turnright" || c == "tr") {
        ws.sendMoveStart("turnright");
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
    }
    // v3: Stride
    else if (c.startsWith("stride ")) {
        float stride = c.substring(7).toFloat();
        ws.sendSetStride(stride);
        Serial.printf("[CMD] Stride set to %.2f\n", stride);
    }
    // v3: SubSteps
    else if (c.startsWith("substeps ")) {
        int steps = c.substring(9).toInt();
        ws.sendSetSubSteps(steps);
        Serial.printf("[CMD] SubSteps set to %d\n", steps);
    }
    // v3: Servo Offset
    else if (c.startsWith("offset ")) {
        int servo, offset;
        if (sscanf(c.c_str(), "offset %d %d", &servo, &offset) == 2) {
            ws.sendSetServoOffset(servo, offset);
            Serial.printf("[CMD] Servo %d offset set to %d\n", servo, offset);
        }
    }
    // v3: Servo Limits
    else if (c.startsWith("limits ")) {
        int servo, minA, maxA, center;
        if (sscanf(c.c_str(), "limits %d %d %d %d", &servo, &minA, &maxA, &center) == 4) {
            ws.sendSetServoLimits(servo, minA, maxA, center);
            Serial.printf("[CMD] Servo %d limits: %d-%d, center=%d\n", servo, minA, maxA, center);
        }
    }
    // v3: Save/Load Calibration
    else if (c == "savecalib") {
        ws.sendSaveCalib();
    } else if (c == "loadcalib") {
        ws.sendLoadCalib();
    }
    // Action commands
    else if (c == "hello" || c == "dance1" || c == "dance2" || c == "dance3" ||
             c == "standby" || c == "sleep") {
        ws.sendCmd(c.c_str());
    }
    // Status
    else if (c == "status") {
        Serial.println(F("\n=== STATUS ==="));
        Serial.printf("WiFi: %s (%s)\n", 
            WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected",
            activeSSID ? activeSSID : "none");
        Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("WebSocket: %s\n", ws.connected() ? "Connected" : "Disconnected");
        
        const WalkParams& wp = ws.getWalkParams();
        Serial.printf("WalkParams: stride=%.2f, subSteps=%d, profile=%s\n",
            wp.stride, wp.subSteps, wp.getProfileName());
        Serial.println(F("==============\n"));
    }
    // Help
    else if (c == "help" || c == "?") {
        printHelp();
    }
    else {
        Serial.printf("[CMD] Unknown: '%s'\n", c.c_str());
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

// =============================================================================
// Setup
// =============================================================================
void setup() {
    Serial.begin(115200);
    delay(100);
    
    Serial.println(F("\n========================================"));
    Serial.println(F("  SpiderRemote v3 - Starting..."));
    Serial.println(F("========================================\n"));
    
    // ADC Konfiguration
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
    
    // I2C für OLED
    Wire.begin(OLED_SDA, OLED_SCL);
    
    // Buttons initialisieren
    btnLeft.begin(PIN_BTN_LEFT, BTN_DEBOUNCE_MS, BTN_LONGPRESS_MS);
    btnMid.begin(PIN_BTN_MID, BTN_DEBOUNCE_MS, BTN_LONGPRESS_MS);
    btnRight.begin(PIN_BTN_RIGHT, BTN_DEBOUNCE_MS, BTN_LONGPRESS_MS);
    btnStop.begin(PIN_BTN_STOP, BTN_DEBOUNCE_MS, BTN_LONGPRESS_MS);
    
    // OLED initialisieren
    oled.begin(OLED_UPDATE_MS);
    
    // MPU6050 (optional)
    if (motion.begin(Wire)) {
        Serial.println(F("[Motion] MPU6050 found"));
    } else {
        Serial.println(F("[Motion] MPU6050 not found"));
    }
    
    // WiFi verbinden
    connectWifiWithFallback();
    
    // WebSocket starten
    ws.begin(activeHost, SPIDER_PORT, SPIDER_PATH, MIN_SEND_INTERVAL_MS);
    
    // Inputs initialisieren
    inputs.begin(PIN_JOY_X, PIN_JOY_Y, PIN_POT_VMAX, PIN_POT_TURN, ADC_MAX);
    inputCalib.begin();
    
    // v3 Controller initialisieren
    drive.begin(&ws);
    uiMenu.begin(&ws);
    
    // WebSocket Text-Callback für Responses vom Bot
    ws.setTextCallback([](const char* json) {
        ws.processResponse(json);
    });
    
    // Servo-Kalibrierung Callback
    ws.setServoCalibCallback([](uint8_t servo, const ServoCalibParams& params) {
        uiMenu.onServoCalibReceived(servo, params);
    });
    
#if SERIAL_CMD_MODE
    printHelp();
#endif
    
    Serial.println(F("\n========================================"));
    Serial.println(F("  SpiderRemote v3 - Ready!"));
    Serial.println(F("========================================\n"));
}

// =============================================================================
// Loop
// =============================================================================
void loop() {
    uint32_t now = millis();
    
    // WiFi-Reconnect Check
    if (now - lastWifiCheck > WIFI_RECONNECT_INTERVAL_MS) {
        lastWifiCheck = now;
        bool isConnected = (WiFi.status() == WL_CONNECTED);
        
        if (!isConnected) {
            Serial.println(F("[WiFi] Lost connection, reconnecting..."));
            connectWifiWithFallback();
            ws.begin(activeHost, SPIDER_PORT, SPIDER_PATH, MIN_SEND_INTERVAL_MS);
        } else if (!wasConnected && isConnected) {
            ws.reconnect();
        }
        wasConnected = isConnected;
    }
    
    // WebSocket verarbeiten
    ws.loop();
    
#if SERIAL_CMD_MODE
    processSerial();
#endif
    
    // Buttons updaten
    btnLeft.update();
    btnMid.update();
    btnRight.update();
    btnStop.update();
    
    // Events konsumieren
    PressType evL = btnLeft.consume();
    PressType evM = btnMid.consume();
    PressType evR = btnRight.consume();
    PressType evStop = btnStop.consume();
    
    // STOP Button hat höchste Priorität
    if (evStop != PressType::None) {
        drive.forceStop();
    }
    
    // UI-State holen
    UiStateV3& state = uiMenu.getState();
    
    // MPU Kalibrierung updaten (wenn aktiv)
    CalibState calibState = motion.getCalibState();
    if (calibState == CalibState::WAITING || calibState == CalibState::RUNNING) {
        motion.updateCalibration();
    }
    
    // Nach erfolgreicher Kalibrierung: DONE-State nach 2s zurücksetzen
    static uint32_t calibDoneTime = 0;
    if (calibState == CalibState::DONE) {
        if (calibDoneTime == 0) {
            calibDoneTime = now;
        } else if (now - calibDoneTime > 2000) {
            motion.resetCalibState();
            calibDoneTime = 0;
        }
    } else {
        calibDoneTime = 0;
    }
    
    // Button-Events an Menü weiterleiten
    if (evL == PressType::Short) uiMenu.onButtonLeft();
    if (evR == PressType::Short) uiMenu.onButtonRight();
    
    // MID-Button: im MOTION-Mode Kalibrierung starten, im INPUT_CALIB Mode weiter
    if (evM == PressType::Short) {
        if (state.mode == MenuModeV3::MOTION && motion.isAvailable()) {
            if (calibState == CalibState::IDLE || calibState == CalibState::DONE || calibState == CalibState::FAILED) {
                motion.startCalibration();
                Serial.println(F("[Motion] Calibration started"));
            }
        } else if (state.mode == MenuModeV3::INPUT_CALIB) {
            InputCalibStep step = inputCalib.getStep();
            if (step == InputCalibStep::IDLE) {
                inputCalib.startCalibration();
                Serial.println(F("[InputCalib] Started"));
            } else {
                inputCalib.nextStep();
            }
        } else {
            uiMenu.onButtonMiddle();
        }
    }
    
    // L/R Buttons: im INPUT_CALIB DEADBAND Mode Deadband anpassen
    if (state.mode == MenuModeV3::INPUT_CALIB && inputCalib.getStep() == InputCalibStep::DEADBAND) {
        if (evL == PressType::Short) {
            inputCalib.adjustDeadband(-1);
        }
        if (evR == PressType::Short) {
            inputCalib.adjustDeadband(1);
        }
    }
    
    // Long-Press MID = Mode wechseln (Haupt-Modi durchschalten)
    if (evM == PressType::Long) {
        // Stoppen bei Mode-Wechsel
        drive.forceStop();
        
        // Nächster Haupt-Modus
        switch (state.mode) {
            case MenuModeV3::DRIVE:
                state.mode = MenuModeV3::MOTION;
                break;
            case MenuModeV3::MOTION:
                state.mode = MenuModeV3::INPUT_CALIB;
                break;
            case MenuModeV3::INPUT_CALIB:
                state.mode = MenuModeV3::WALK_PARAMS;
                break;
            case MenuModeV3::WALK_PARAMS:
                state.mode = MenuModeV3::SERVO_CALIB;
                break;
            case MenuModeV3::SERVO_CALIB:
            case MenuModeV3::SERVO_SELECT:
            case MenuModeV3::SERVO_OFFSET:
            case MenuModeV3::SERVO_LIMITS:
                state.mode = MenuModeV3::ACTIONS;
                break;
            case MenuModeV3::ACTIONS:
                state.mode = MenuModeV3::TERRAIN;
                break;
            case MenuModeV3::TERRAIN:
            default:
                state.mode = MenuModeV3::DRIVE;
                break;
        }
        state.needsRedraw = true;
        Serial.printf("[UI] Mode -> %d\n", (int)state.mode);
    }
    
    // Motion-Status im UI-State aktualisieren
    state.motionAvailable = motion.isAvailable();
    state.motionCalibrated = motion.isCalibrated();
    state.motionCalibState = motion.getCalibState();
    state.motionCalibProgress = motion.getCalibProgress();
    
    // Input-Kalibrierung Status aktualisieren
    state.inputCalibStep = inputCalib.getStep();
    state.inputCalibText = inputCalib.getStepText();
    state.inputDeadbandPercent = inputCalib.getDeadbandPercent();
    state.inputCalibrated = inputCalib.isCalibrated();
    
    // Input-Kalibrierung: Raw-Werte lesen und update aufrufen
    if (state.mode == MenuModeV3::INPUT_CALIB && 
        state.inputCalibStep != InputCalibStep::IDLE &&
        state.inputCalibStep != InputCalibStep::DONE) {
        int rawX, rawY, rawT, rawV;
        inputs.readRaw(rawX, rawY, rawT, rawV);
        inputCalib.update(rawX, rawY, rawT, rawV);
    }
    
    // Input-Kalibrierung: DONE nach 2s zurücksetzen
    static uint32_t inputCalibDoneTime = 0;
    if (state.inputCalibStep == InputCalibStep::DONE) {
        if (inputCalibDoneTime == 0) {
            inputCalibDoneTime = now;
        } else if (now - inputCalibDoneTime > 2000) {
            inputCalib.nextStep();  // DONE -> IDLE
            inputCalibDoneTime = 0;
        }
    } else {
        inputCalibDoneTime = 0;
    }
    
    // Drive Control im DRIVE oder MOTION-Modus
    InputState in = {SPEED_MIN, 0, 0, 0};
    
    // Speed-Poti immer lesen
    int speedFromPot = inputs.readSpeedPotRaw(SPEED_MIN, SPEED_MAX);
    in.speedMax = speedFromPot;
    
    // Speed senden
    static int lastSentSpeed = -1;
    if (speedFromPot != lastSentSpeed && ws.connected()) {
        ws.sendSetSpeed(speedFromPot);
        lastSentSpeed = speedFromPot;
    }
    
    if (state.mode == MenuModeV3::DRIVE) {
        // Joystick lesen
        if (inputCalib.isCalibrated()) {
            in = inputs.readCalibrated(inputCalib.getData(), SPEED_MIN, SPEED_MAX);
        } else {
            in = inputs.read(DEAD_JOY, DEAD_TURN, SPEED_MIN, SPEED_MAX);
        }
        in.speedMax = speedFromPot;
        drive.tick(in, SPEED_MIN);
    }
    else if (state.mode == MenuModeV3::MOTION) {
        // MPU-Steuerung (nur für Neigung, Turn vom Poti)
        if (motion.isAvailable() && motion.isCalibrated() && 
            motion.getCalibState() != CalibState::RUNNING) {
            in = motion.read(speedFromPot);
            // Turn vom Poti überschreiben (MPU-Gyro ist problematisch)
            InputState potiIn = inputs.read(DEAD_JOY, DEAD_TURN, SPEED_MIN, SPEED_MAX);
            in.turn = potiIn.turn;
            drive.tick(in, SPEED_MIN);
        }
    }
    
    // Aktuelle Bewegung im State speichern
    state.currentMove = drive.currentMove();
    
    // Poti für Parameter-Änderung in bestimmten Menüs
    if (state.mode == MenuModeV3::WALK_PARAMS || 
        state.mode == MenuModeV3::SERVO_OFFSET) {
        static int lastPotiValue = -1;
        int potiValue = map(speedFromPot, SPEED_MIN, SPEED_MAX, 0, 100);
        if (abs(potiValue - lastPotiValue) > 3) {
            uiMenu.onPotiChange(potiValue);
            lastPotiValue = potiValue;
        }
    }
    
    // Display updaten mit v3-Menü
    char line1[22], line2[22], line3[22], line4[22];
    int progressBar = -1;
    uiMenu.getDisplayLines(line1, line2, line3, line4, progressBar);
    
    oled.tickV3(ws.connected(), in.speedMax, line1, line2, line3, line4, progressBar);
}
