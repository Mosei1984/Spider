#include "RobotController.h"
#include "../motion/MotionData.h"

RobotController robotController;

RobotController::RobotController() 
    : currentCmd(MotionCmd::NONE), 
      pendingCmd(MotionCmd::NONE),
      hasPendingCmd(false),
      continuousMode(false), 
      stopAfterSequence(false),
      motionRunning(false),
      calibrationLocked(true) {}

MotionCmd RobotController::parseCommand(const char* name) {
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
    
    return MotionCmd::NONE;
}

const char* RobotController::getCommandName(MotionCmd cmd) {
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
        default: return "none";
    }
}

bool RobotController::isContinuousCmd(MotionCmd cmd) {
    return (cmd == MotionCmd::FORWARD || 
            cmd == MotionCmd::BACKWARD ||
            cmd == MotionCmd::LEFT || 
            cmd == MotionCmd::RIGHT ||
            cmd == MotionCmd::TURNLEFT || 
            cmd == MotionCmd::TURNRIGHT);
}

void RobotController::queueCommand(MotionCmd cmd) {
    if (cmd == MotionCmd::NONE) return;
    
    noInterrupts();
    pendingCmd = cmd;
    hasPendingCmd = true;
    // Bei neuen Befehlen continuous mode beenden
    if (continuousMode) {
        continuousMode = false;
        stopAfterSequence = false;
    }
    interrupts();
    Serial.printf("[Robot] Command queued: %s\n", getCommandName(cmd));
}

void RobotController::startContinuous(MotionCmd cmd) {
    if (!isContinuousCmd(cmd)) {
        // Kein kontinuierlicher Befehl - normal ausführen
        queueCommand(cmd);
        return;
    }
    
    Serial.printf("[Robot] Kontinuierlich starten: %s\n", getCommandName(cmd));
    
    // Atomares Setzen der Flags
    noInterrupts();
    stopAfterSequence = false;
    continuousMode = true;
    currentCmd = cmd;
    interrupts();
}

void RobotController::requestStop() {
    if (!continuousMode) return;

    if (!motionRunning) {
        // Keine Motion aktiv -> sofort zu Standby
        noInterrupts();
        continuousMode = false;
        stopAfterSequence = false;
        currentCmd = MotionCmd::STANDBY;
        interrupts();
        startMotionForCmd(MotionCmd::STANDBY);
        Serial.println("[Robot] Stop (sofort zu Standby)");
        return;
    }

    stopAfterSequence = true;
    Serial.println("[Robot] Stop nach Sequenz angefordert");
}

void RobotController::forceStop() {
    Serial.println("[Robot] Force Stop!");
    motion_stop();
    
    noInterrupts();
    continuousMode = false;
    stopAfterSequence = false;
    motionRunning = false;
    hasPendingCmd = false;
    currentCmd = MotionCmd::STANDBY;
    pendingCmd = MotionCmd::NONE;
    interrupts();
    
    // Standby nicht-blockierend starten
    startMotionForCmd(MotionCmd::STANDBY);
}

void RobotController::startMotionForCmd(MotionCmd cmd) {
    currentCmd = cmd;
    motion_reset_sequence_flag();
    motionRunning = true;
    
    switch (cmd) {
        case MotionCmd::FORWARD:   motion_start(Servo_Prg_2, Servo_Prg_2_Step); break;
        case MotionCmd::BACKWARD:  motion_start(Servo_Prg_3, Servo_Prg_3_Step); break;
        case MotionCmd::LEFT:      motion_start(Servo_Prg_4, Servo_Prg_4_Step); break;
        case MotionCmd::RIGHT:     motion_start(Servo_Prg_5, Servo_Prg_5_Step); break;
        case MotionCmd::TURNLEFT:  motion_start(Servo_Prg_6, Servo_Prg_6_Step); break;
        case MotionCmd::TURNRIGHT: motion_start(Servo_Prg_7, Servo_Prg_7_Step); break;
        case MotionCmd::STANDBY:   motion_start(Servo_Prg_1, Servo_Prg_1_Step); break;
        default: motionRunning = false; break;
    }
}

void RobotController::executeCommandBlocking(MotionCmd cmd) {
    currentCmd = cmd;
    
    switch (cmd) {
        case MotionCmd::FORWARD:   forward();   break;
        case MotionCmd::BACKWARD:  back();      break;
        case MotionCmd::LEFT:      leftmove();  break;
        case MotionCmd::RIGHT:     rightmove(); break;
        case MotionCmd::TURNLEFT:  turnleft();  break;
        case MotionCmd::TURNRIGHT: turnright(); break;
        case MotionCmd::STANDBY:   standby();   break;
        case MotionCmd::SLEEP:     sleep();     break;
        case MotionCmd::LIE:       lie();       break;
        case MotionCmd::HELLO:     hello();     break;
        case MotionCmd::PUSHUP:    pushup();    break;
        case MotionCmd::FIGHTING:  fighting();  break;
        case MotionCmd::DANCE1:    dance1();    break;
        case MotionCmd::DANCE2:    dance2();    break;
        case MotionCmd::DANCE3:    dance3();    break;
        default: break;
    }
}

void RobotController::processQueue() {
    unsigned long now = millis();
    
    // Motion Engine ticken wenn aktiv (terrain_blend_tick wird in motion_tick aufgerufen)
    if (motionRunning) {
        if (motion_tick(now)) {
            return;  // Motion läuft noch
        }
        
        // Motion-Sequenz beendet
        motionRunning = false;
        
        // Pending Command hat Priorität über continuous repeat
        if (hasPendingCmd) {
            // Wird unten verarbeitet
        }
        // Stop nach letzter Sequenz
        else if (stopAfterSequence) {
            noInterrupts();
            stopAfterSequence = false;
            continuousMode = false;
            currentCmd = MotionCmd::STANDBY;
            interrupts();
            startMotionForCmd(MotionCmd::STANDBY);
            return;
        }
        // Kontinuierlicher Modus: Sequenz erneut starten
        else if (continuousMode) {
            Serial.printf("[Robot] Sequenz wiederholen: %s\n", getCommandName(currentCmd));
            startMotionForCmd(currentCmd);
            return;
        }
    } else {
        // Terrain nur ticken wenn keine Motion läuft (motion_tick ruft es bereits auf)
        terrain_blend_tick();
    }
    
    // Pending Commands haben höchste Priorität
    if (hasPendingCmd && !motionRunning) {
        noInterrupts();
        hasPendingCmd = false;
        MotionCmd cmd = pendingCmd;
        pendingCmd = MotionCmd::NONE;
        interrupts();
        
        // STANDBY auch nicht-blockierend ausführen
        if (cmd == MotionCmd::STANDBY || isContinuousCmd(cmd)) {
            startMotionForCmd(cmd);
        } else {
            executeCommandBlocking(cmd);
        }
        return;
    }
    
    // Neue kontinuierliche Bewegung starten (nur wenn kein pending)
    if (continuousMode && !motionRunning && !stopAfterSequence) {
        startMotionForCmd(currentCmd);
        return;
    }
}

int RobotController::getServoOffset(int index) {
    return ::getServoOffset(index);
}

bool RobotController::setServoOffset(int index, int value) {
    if (calibrationLocked) {
        Serial.println("[Robot] Calibration locked - offset change rejected");
        return false;
    }
    ::setServoOffset(index, value);
    return true;
}

void RobotController::setCalibrationLocked(bool locked) {
    calibrationLocked = locked;
    Serial.printf("[Robot] Calibration lock: %s\n", locked ? "ON" : "OFF");
}
