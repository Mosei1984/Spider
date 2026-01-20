#include "RobotController.h"
#include "../motion/MotionData.h"

RobotController robotController;

RobotController::RobotController() 
    : currentCmd(MotionCmd::NONE), 
      pendingCmd(MotionCmd::NONE),
      hasPendingCmd(false),
      continuousMode(false), 
      stopAfterSequence(false) {}

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
    
    pendingCmd = cmd;
    hasPendingCmd = true;
    Serial.printf("[Robot] Command queued: %s\n", getCommandName(cmd));
}

void RobotController::startContinuous(MotionCmd cmd) {
    if (!isContinuousCmd(cmd)) {
        // Kein kontinuierlicher Befehl - normal ausführen
        queueCommand(cmd);
        return;
    }
    
    Serial.printf("[Robot] Kontinuierlich starten: %s\n", getCommandName(cmd));
    
    // NUR Flags setzen - NICHT executeCommand() aufrufen!
    // Die Ausführung passiert in processQueue() im loop()
    stopAfterSequence = false;
    continuousMode = true;
    currentCmd = cmd;
}

void RobotController::requestStop() {
    if (continuousMode) {
        stopAfterSequence = true;
        Serial.println("[Robot] Stop nach Sequenz angefordert");
    }
}

void RobotController::forceStop() {
    Serial.println("[Robot] Force Stop!");
    motion_stop();
    continuousMode = false;
    stopAfterSequence = false;
    hasPendingCmd = false;
    
    // Standby als nächsten Befehl queuen (wird in processQueue ausgeführt)
    currentCmd = MotionCmd::STANDBY;
    pendingCmd = MotionCmd::STANDBY;
    hasPendingCmd = true;
}

void RobotController::executeCommand(MotionCmd cmd) {
    currentCmd = cmd;
    motion_reset_sequence_flag();
    
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
    // Terrain sanft anpassen (bei jedem Aufruf)
    terrain_blend_tick();
    
    // Kontinuierliche Bewegung fortsetzen
    if (continuousMode && !stopAfterSequence) {
        executeCommand(currentCmd);
        return;
    }
    
    // Stop nach letzter Sequenz
    if (stopAfterSequence) {
        stopAfterSequence = false;
        continuousMode = false;
        currentCmd = MotionCmd::STANDBY;
        standby();
        return;
    }
    
    // Normale Queue-Verarbeitung
    if (hasPendingCmd) {
        hasPendingCmd = false;
        MotionCmd cmd = pendingCmd;
        pendingCmd = MotionCmd::NONE;
        executeCommand(cmd);
    }
}

bool RobotController::isBusy() {
    return continuousMode;
}

bool RobotController::isContinuousActive() {
    return continuousMode && !stopAfterSequence;
}

String RobotController::getStatus() {
    String json = "{";
    json += "\"running\":" + String(motion_is_active() ? "true" : "false") + ",";
    json += "\"continuous\":" + String(continuousMode ? "true" : "false") + ",";
    json += "\"currentCmd\":\"" + String(getCommandName(currentCmd)) + "\",";
    json += "\"terrain\":\"" + String(getTerrainModeName()) + "\",";
    json += "\"terrainBlend\":" + String(terrainBlendCurrent) + ",";
    json += "\"offsets\":[";
    for (int i = 0; i < ALLSERVOS; i++) {
        if (i > 0) json += ",";
        json += String(Servo_Offset[i]);
    }
    json += "],";
    json += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"uptime\":" + String(millis());
    json += "}";
    return json;
}

int RobotController::getServoOffset(int index) {
    return ::getServoOffset(index);
}

void RobotController::setServoOffset(int index, int value) {
    ::setServoOffset(index, value);
}

void RobotController::resetServoOffsets() {
    ::resetServoOffsets();
}
