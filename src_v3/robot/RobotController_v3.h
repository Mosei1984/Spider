// =============================================================================
// RobotController_v3.h - Robot Controller mit GaitRuntime Integration
// =============================================================================
// v3 Robot Controller für ESP8266 Spider
// Features:
//   - GaitRuntime Integration für nicht-blockierende Bewegung
//   - Walk-Parameter (stride, subSteps, timing) Unterstützung
//   - Kalibrierungs-Integration
//   - Erweitertes Command-Handling
// =============================================================================
#ifndef ROBOT_CONTROLLER_V3_H
#define ROBOT_CONTROLLER_V3_H

#include <Arduino.h>
#include "../gait/GaitConfig.h"

// =============================================================================
// Motion Commands
// =============================================================================
enum class MotionCmd : uint8_t {
    NONE = 0,
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    TURNLEFT,
    TURNRIGHT,
    STANDBY,
    SLEEP,
    LIE,
    HELLO,
    PUSHUP,
    FIGHTING,
    DANCE1,
    DANCE2,
    DANCE3,
    CALIBPOSE
};

// =============================================================================
// Walk-Parameter für erweitertes Protokoll
// =============================================================================
struct WalkParams {
    float stride;           // Stride-Faktor (0.3 - 2.0)
    uint8_t subSteps;       // Substeps (1-16)
    TimingProfile profile;  // Timing-Profil
    float swingMul;         // Swing-Multiplier
    float stanceMul;        // Stance-Multiplier
    bool rampEnabled;       // Soft-Start aktiviert
    uint8_t rampCycles;     // Ramp-Zyklen
    
    WalkParams() 
        : stride(1.0f)
        , subSteps(8)
        , profile(TimingProfile::SWING_STANCE)
        , swingMul(0.75f)
        , stanceMul(1.25f)
        , rampEnabled(false)
        , rampCycles(3) {}
};

// =============================================================================
// Robot Controller Klasse
// =============================================================================
class RobotControllerV3 {
public:
    RobotControllerV3();
    
    // Command Parsing
    MotionCmd parseCommand(const char* name);
    const char* getCommandName(MotionCmd cmd);
    bool isContinuousCmd(MotionCmd cmd);
    
    // Command Queue
    void queueCommand(MotionCmd cmd);
    void startContinuous(MotionCmd cmd);
    void requestStop();
    void forceStop();
    
    // Haupt-Prozessschleife (in loop() aufrufen)
    void processQueue();
    
    // Walk-Parameter setzen (vom Remote)
    void setWalkParams(const WalkParams& params);
    const WalkParams& getWalkParams() const { return walkParams; }
    
    // Live-Parameter-Änderung
    void setStrideFactor(float factor);
    void setSubSteps(uint8_t steps);
    void setTimingProfile(TimingProfile profile);
    void setSwingMultiplier(float mult);
    void setStanceMultiplier(float mult);
    void enableRamp(bool enable, uint8_t cycles = 3);
    
    // Kalibrierungs-Lock
    void setCalibrationLocked(bool locked);
    bool isCalibrationLocked() const { return calibrationLocked; }
    
    // Status
    bool isMoving() const { return motionRunning; }
    bool isContinuousMode() const { return continuousMode; }
    MotionCmd getCurrentCmd() const { return currentCmd; }
    
private:
    // Motion für Command starten (mit GaitRuntime)
    void startMotionForCmd(MotionCmd cmd);
    
    // Blockierende Commands ausführen (Legacy)
    void executeCommandBlocking(MotionCmd cmd);
    
    // Walk-Parameter auf GaitRuntime anwenden
    void applyWalkParams();
    
    // State
    MotionCmd currentCmd;
    MotionCmd pendingCmd;
    bool hasPendingCmd;
    bool continuousMode;
    bool stopAfterSequence;
    bool motionRunning;
    bool calibrationLocked;
    
    // Walk-Parameter
    WalkParams walkParams;
};

// Globale Instanz
extern RobotControllerV3 robotController;

#endif // ROBOT_CONTROLLER_V3_H
