// =============================================================================
// RobotController_v3.cpp - Robot Controller Implementierung
// =============================================================================
#include "RobotController_v3.h"
#include "../motion/MotionData_v3.h"
#include "../gait/GaitRuntime.h"
#include "../calibration/ServoCalibration.h"

// Globale Instanz
RobotControllerV3 robotController;

// =============================================================================
// Konstruktor
// =============================================================================
RobotControllerV3::RobotControllerV3() 
    : currentCmd(MotionCmd::NONE)
    , pendingCmd(MotionCmd::NONE)
    , hasPendingCmd(false)
    , continuousMode(false)
    , stopAfterSequence(false)
    , motionRunning(false)
    , calibrationLocked(true)
    , walkParams() {}

// =============================================================================
// Command Parsing
// =============================================================================
MotionCmd RobotControllerV3::parseCommand(const char* name) {
    if (!name) return MotionCmd::NONE;
    
    String cmd = String(name);
    cmd.toLowerCase();
    
    if (cmd == "forward") return MotionCmd::FORWARD;
    if (cmd == "back" || cmd == "backward") return MotionCmd::BACKWARD;
    if (cmd == "left" || cmd == "leftmove") return MotionCmd::LEFT;
    if (cmd == "right" || cmd == "rightmove") return MotionCmd::RIGHT;
    if (cmd == "turnleft") return MotionCmd::TURNLEFT;
    if (cmd == "turnright") return MotionCmd::TURNRIGHT;
    if (cmd == "standby") return MotionCmd::STANDBY;
    if (cmd == "sleep") return MotionCmd::SLEEP;
    if (cmd == "lie") return MotionCmd::LIE;
    if (cmd == "hello") return MotionCmd::HELLO;
    if (cmd == "pushup") return MotionCmd::PUSHUP;
    if (cmd == "fighting") return MotionCmd::FIGHTING;
    if (cmd == "dance1") return MotionCmd::DANCE1;
    if (cmd == "dance2") return MotionCmd::DANCE2;
    if (cmd == "dance3") return MotionCmd::DANCE3;
    if (cmd == "calibpose") return MotionCmd::CALIBPOSE;
    
    return MotionCmd::NONE;
}

const char* RobotControllerV3::getCommandName(MotionCmd cmd) {
    switch (cmd) {
        case MotionCmd::FORWARD: return "forward";
        case MotionCmd::BACKWARD: return "backward";
        case MotionCmd::LEFT: return "left";
        case MotionCmd::RIGHT: return "right";
        case MotionCmd::TURNLEFT: return "turnleft";
        case MotionCmd::TURNRIGHT: return "turnright";
        case MotionCmd::STANDBY: return "standby";
        case MotionCmd::SLEEP: return "sleep";
        case MotionCmd::LIE: return "lie";
        case MotionCmd::HELLO: return "hello";
        case MotionCmd::PUSHUP: return "pushup";
        case MotionCmd::FIGHTING: return "fighting";
        case MotionCmd::DANCE1: return "dance1";
        case MotionCmd::DANCE2: return "dance2";
        case MotionCmd::DANCE3: return "dance3";
        case MotionCmd::CALIBPOSE: return "calibpose";
        default: return "none";
    }
}

bool RobotControllerV3::isContinuousCmd(MotionCmd cmd) {
    return (cmd == MotionCmd::FORWARD || 
            cmd == MotionCmd::BACKWARD ||
            cmd == MotionCmd::LEFT || 
            cmd == MotionCmd::RIGHT ||
            cmd == MotionCmd::TURNLEFT || 
            cmd == MotionCmd::TURNRIGHT);
}

// =============================================================================
// Command Queue
// =============================================================================
void RobotControllerV3::queueCommand(MotionCmd cmd) {
    if (cmd == MotionCmd::NONE) return;
    
    noInterrupts();
    pendingCmd = cmd;
    hasPendingCmd = true;
    if (continuousMode) {
        continuousMode = false;
        stopAfterSequence = false;
    }
    interrupts();
    Serial.printf("[RobotV3] Command queued: %s\n", getCommandName(cmd));
}

void RobotControllerV3::startContinuous(MotionCmd cmd) {
    if (!isContinuousCmd(cmd)) {
        queueCommand(cmd);
        return;
    }
    
    Serial.printf("[RobotV3] Kontinuierlich starten: %s\n", getCommandName(cmd));
    
    // Walk-Parameter auf GaitRuntime anwenden
    applyWalkParams();
    
    noInterrupts();
    stopAfterSequence = false;
    continuousMode = true;
    currentCmd = cmd;
    interrupts();
}

void RobotControllerV3::requestStop() {
    if (!continuousMode) return;
    
    if (!motionRunning) {
        noInterrupts();
        continuousMode = false;
        stopAfterSequence = false;
        currentCmd = MotionCmd::STANDBY;
        interrupts();
        startMotionForCmd(MotionCmd::STANDBY);
        Serial.println(F("[RobotV3] Stop (sofort zu Standby)"));
        return;
    }
    
    stopAfterSequence = true;
    Serial.println(F("[RobotV3] Stop nach Sequenz angefordert"));
}

void RobotControllerV3::forceStop() {
    Serial.println(F("[RobotV3] Force Stop!"));
    GaitRuntime::stop();
    
    noInterrupts();
    continuousMode = false;
    stopAfterSequence = false;
    motionRunning = false;
    hasPendingCmd = false;
    currentCmd = MotionCmd::STANDBY;
    pendingCmd = MotionCmd::NONE;
    interrupts();
    
    startMotionForCmd(MotionCmd::STANDBY);
}

// =============================================================================
// Motion starten mit GaitRuntime
// =============================================================================
void RobotControllerV3::startMotionForCmd(MotionCmd cmd) {
    currentCmd = cmd;
    GaitRuntime::resetSequenceFlag();
    motionRunning = true;
    
    switch (cmd) {
        case MotionCmd::FORWARD:   GaitRuntime::start(Servo_Prg_2, Servo_Prg_2_Step); break;
        case MotionCmd::BACKWARD:  GaitRuntime::start(Servo_Prg_3, Servo_Prg_3_Step); break;
        case MotionCmd::LEFT:      GaitRuntime::start(Servo_Prg_4, Servo_Prg_4_Step); break;
        case MotionCmd::RIGHT:     GaitRuntime::start(Servo_Prg_5, Servo_Prg_5_Step); break;
        case MotionCmd::TURNLEFT:  GaitRuntime::start(Servo_Prg_6, Servo_Prg_6_Step); break;
        case MotionCmd::TURNRIGHT: GaitRuntime::start(Servo_Prg_7, Servo_Prg_7_Step); break;
        case MotionCmd::STANDBY:   GaitRuntime::start(Servo_Prg_1, Servo_Prg_1_Step); break;
        default: motionRunning = false; break;
    }
}

void RobotControllerV3::executeCommandBlocking(MotionCmd cmd) {
    currentCmd = cmd;
    
    switch (cmd) {
        case MotionCmd::FORWARD:   forward_blocking();   break;
        case MotionCmd::BACKWARD:  back_blocking();      break;
        case MotionCmd::LEFT:      leftmove_blocking();  break;
        case MotionCmd::RIGHT:     rightmove_blocking(); break;
        case MotionCmd::TURNLEFT:  turnleft_blocking();  break;
        case MotionCmd::TURNRIGHT: turnright_blocking(); break;
        case MotionCmd::STANDBY:   standby();   break;
        case MotionCmd::SLEEP:     sleep();     break;
        case MotionCmd::LIE:       lie();       break;
        case MotionCmd::HELLO:     hello();     break;
        case MotionCmd::PUSHUP:    pushup();    break;
        case MotionCmd::FIGHTING:  fighting();  break;
        case MotionCmd::DANCE1:    dance1();    break;
        case MotionCmd::DANCE2:    dance2();    break;
        case MotionCmd::DANCE3:    dance3();    break;
        case MotionCmd::CALIBPOSE: calibpose(); break;
        default: break;
    }
}

// =============================================================================
// Haupt-Prozessschleife
// =============================================================================
void RobotControllerV3::processQueue() {
    unsigned long now = millis();
    
    // GaitRuntime ticken wenn Motion aktiv
    if (motionRunning) {
        if (GaitRuntime::tick(now)) {
            return;  // Motion läuft noch
        }
        
        // Motion-Sequenz beendet
        motionRunning = false;
        
        // Pending Command prüfen
        if (hasPendingCmd) {
            // Wird unten verarbeitet
        }
        // Stop nach Sequenz
        else if (stopAfterSequence) {
            noInterrupts();
            stopAfterSequence = false;
            continuousMode = false;
            currentCmd = MotionCmd::STANDBY;
            interrupts();
            startMotionForCmd(MotionCmd::STANDBY);
            return;
        }
        // Kontinuierlich: Sequenz wiederholen
        else if (continuousMode) {
            Serial.printf("[RobotV3] Sequenz wiederholen: %s\n", getCommandName(currentCmd));
            startMotionForCmd(currentCmd);
            return;
        }
    } else {
        // Terrain ticken wenn keine Motion läuft
        terrain_blend_tick();
    }
    
    // Pending Commands verarbeiten
    if (hasPendingCmd && !motionRunning) {
        noInterrupts();
        hasPendingCmd = false;
        MotionCmd cmd = pendingCmd;
        pendingCmd = MotionCmd::NONE;
        interrupts();
        
        // Walk-Parameter anwenden bei kontinuierlichen Commands
        if (isContinuousCmd(cmd)) {
            applyWalkParams();
        }
        
        if (cmd == MotionCmd::STANDBY || isContinuousCmd(cmd)) {
            startMotionForCmd(cmd);
        } else {
            executeCommandBlocking(cmd);
        }
        return;
    }
    
    // Neue kontinuierliche Bewegung starten
    if (continuousMode && !motionRunning && !stopAfterSequence) {
        startMotionForCmd(currentCmd);
        return;
    }
}

// =============================================================================
// Walk-Parameter
// =============================================================================
void RobotControllerV3::setWalkParams(const WalkParams& params) {
    walkParams = params;
    Serial.printf("[RobotV3] WalkParams: stride=%.2f, subSteps=%d, profile=%d\n",
        params.stride, params.subSteps, (int)params.profile);
    
    // Sofort anwenden wenn Motion läuft
    if (motionRunning) {
        applyWalkParams();
    }
}

void RobotControllerV3::applyWalkParams() {
    GaitRuntime::setStrideFactor(walkParams.stride);
    GaitRuntime::setSubSteps(walkParams.subSteps);
    GaitRuntime::setTimingProfile(walkParams.profile);
    GaitRuntime::setSwingMultiplier(walkParams.swingMul);
    GaitRuntime::setStanceMultiplier(walkParams.stanceMul);
    GaitRuntime::enableRamp(walkParams.rampEnabled, walkParams.rampCycles);
    
    if (walkParams.rampEnabled) {
        GaitRuntime::setTargetStride(walkParams.stride);
    }
}

void RobotControllerV3::setStrideFactor(float factor) {
    walkParams.stride = factor;
    GaitRuntime::setStrideFactor(factor);
}

void RobotControllerV3::setSubSteps(uint8_t steps) {
    walkParams.subSteps = steps;
    GaitRuntime::setSubSteps(steps);
}

void RobotControllerV3::setTimingProfile(TimingProfile profile) {
    walkParams.profile = profile;
    GaitRuntime::setTimingProfile(profile);
}

void RobotControllerV3::setSwingMultiplier(float mult) {
    walkParams.swingMul = mult;
    GaitRuntime::setSwingMultiplier(mult);
}

void RobotControllerV3::setStanceMultiplier(float mult) {
    walkParams.stanceMul = mult;
    GaitRuntime::setStanceMultiplier(mult);
}

void RobotControllerV3::enableRamp(bool enable, uint8_t cycles) {
    walkParams.rampEnabled = enable;
    walkParams.rampCycles = cycles;
    GaitRuntime::enableRamp(enable, cycles);
}

// =============================================================================
// Kalibrierung
// =============================================================================
void RobotControllerV3::setCalibrationLocked(bool locked) {
    calibrationLocked = locked;
    Serial.printf("[RobotV3] Calibration lock: %s\n", locked ? "ON" : "OFF");
}
