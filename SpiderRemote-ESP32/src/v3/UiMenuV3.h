// =============================================================================
// UiMenuV3.h - Erweitertes UI-Menü für v3 Parameter
// =============================================================================
// SpiderRemote-ESP32 v3 Extension
// Neue Menü-Pages für:
//   - Walk-Parameter (Stride, SubSteps, Timing)
//   - Servo-Kalibrierung (Offset, Limits pro Servo)
// =============================================================================
#pragma once
#include <Arduino.h>
#include "WalkParams.h"
#include "WsClientV3.h"
#include "ServoCalibStore.h"
#include "../MotionInputs.h"
#include "../InputCalib.h"

// =============================================================================
// Menü-Modi für v3
// =============================================================================
enum class MenuModeV3 : uint8_t {
    DRIVE = 0,        // Standard-Fahrmodus (Joystick)
    MOTION,           // MPU6050 Motion-Steuerung
    INPUT_CALIB,      // Joystick/Poti Kalibrierung
    WALK_PARAMS,      // Walk-Parameter einstellen
    SERVO_CALIB,      // Servo-Kalibrierung
    SERVO_SELECT,     // Servo auswählen
    SERVO_OFFSET,     // Offset für gewählten Servo
    SERVO_LIMITS,     // Limits für gewählten Servo
    ACTIONS,          // Actions (Hello, Dance, etc.)
    TERRAIN           // Terrain-Modus
};

// =============================================================================
// UI-State für v3
// =============================================================================
struct UiStateV3 {
    MenuModeV3 mode;
    
    // Walk-Parameter aktuell im Menü
    WalkParams walkParams;
    int walkParamIndex;  // 0=stride, 1=subSteps, 2=profile, 3=swingMul, 4=stanceMul, 5=ramp, 6=save, 7=send
    bool walkParamsSaved;
    
    // Servo-Kalibrierung
    uint8_t selectedServo;
    ServoCalibParams servoCalib[8];
    int calibParamIndex;  // 0=offset, 1=min, 2=max, 3=center
    bool servoCalibSaved;  // Lokal gespeichert?
    int servoCalibMenuIndex;  // 0=Select, 1=Grundstellung, 2=Lock/Unlock, 3=Vom Bot laden, 4=Save Local, 5=Send to Bot
    bool calibLocked;  // Kalibrierungs-Lock Status
    bool waitingForBotCalib;  // Warten auf Antwort vom Bot
    uint8_t receivedCalibCount;  // Anzahl empfangener Kalibrierungen
    
    // Actions & Terrain
    int actionIndex;
    int terrainIndex;
    const char* terrainActive;
    
    // Motion (MPU)
    bool motionAvailable;
    bool motionCalibrated;
    CalibState motionCalibState;
    int motionCalibProgress;
    
    // Aktuelle Bewegung (für Anzeige)
    const char* currentMove;
    
    // Input-Kalibrierung
    InputCalibStep inputCalibStep;
    const char* inputCalibText;
    int inputDeadbandPercent;
    bool inputCalibrated;
    
    // Display-Update Flag
    bool needsRedraw;
    
    UiStateV3() {
        mode = MenuModeV3::DRIVE;
        walkParamIndex = 0;
        walkParamsSaved = false;
        selectedServo = 0;
        calibParamIndex = 0;
        servoCalibSaved = false;
        servoCalibMenuIndex = 0;
        calibLocked = true;  // Default: gesperrt
        waitingForBotCalib = false;
        receivedCalibCount = 0;
        actionIndex = 3;  // hello
        terrainIndex = 0;
        terrainActive = "normal";
        motionAvailable = false;
        motionCalibrated = false;
        motionCalibState = CalibState::IDLE;
        motionCalibProgress = 0;
        currentMove = "";
        inputCalibStep = InputCalibStep::IDLE;
        inputCalibText = "";
        inputDeadbandPercent = 10;
        inputCalibrated = false;
        needsRedraw = true;
    }
};

// =============================================================================
// Menü-Controller Klasse
// =============================================================================
class UiMenuV3 {
public:
    void begin(WsClientV3* ws);
    
    // Button-Handler
    void onButtonLeft();
    void onButtonMiddle();
    void onButtonRight();
    
    // Poti-Handler für Wertänderung im aktuellen Menü
    void onPotiChange(int value);  // 0-100
    
    // State-Zugriff
    UiStateV3& getState() { return state; }
    const UiStateV3& getState() const { return state; }
    
    // Display-Text für aktuelle Seite
    // progressBar: -1 = kein Balken, 0-100 = Fortschritt
    void getDisplayLines(char* line1, char* line2, char* line3, char* line4, int& progressBar);
    
    // Parameter an Spider senden
    void applyWalkParams();
    void applyServoCalib(uint8_t servo);
    void saveAllCalib();
    
    // Actions & Terrain
    void executeAction();
    void applyTerrain();
    
    // Servo-Kalibrierung
    void saveServoCalibLocal();
    void sendAllServoCalibToBot();
    void loadServoCalibFromStore();
    void sendHomePosition();
    void toggleCalibLock();
    void requestCalibFromBot();
    void onServoCalibReceived(uint8_t servo, const ServoCalibParams& params);
    
    // Walk-Parameter
    void saveWalkParamsLocal();
    void loadWalkParamsLocal();
    void sendWalkParamsToBot();
    
    // Servo Store Zugriff
    ServoCalibStore& getServoStore() { return servoStore; }
    
private:
    ServoCalibStore servoStore;
    WsClientV3* ws = nullptr;
    UiStateV3 state;
    
    // Menü-Navigation
    void nextMode();
    void prevMode();
    
    // Walk-Parameter Menü
    void walkParamUp();
    void walkParamDown();
    void walkParamIncrease();
    void walkParamDecrease();
    
    // Servo-Kalibrierung Menü
    void servoSelectUp();
    void servoSelectDown();
    void calibParamUp();
    void calibParamDown();
    void calibValueIncrease();
    void calibValueDecrease();
    
    // Helper
    const char* getWalkParamName(int index);
    const char* getCalibParamName(int index);
};
