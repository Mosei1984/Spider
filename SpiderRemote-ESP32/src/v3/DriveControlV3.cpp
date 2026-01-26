// =============================================================================
// DriveControlV3.cpp - Implementierung des erweiterten Drive Controllers
// =============================================================================
#include "DriveControlV3.h"

void DriveControlV3::begin(WsClientV3* wsClient) {
    ws = wsClient;
    walkParams = WalkParams();  // Defaults
}

void DriveControlV3::tick(const InputState& in, int speedMin) {
    // Bewegungsrichtung bestimmen
    MoveDir desiredMove = MoveDir::None;
    
    // Schwellwerte (floats -1..+1)
    const float DEAD_ZONE = 0.15f;
    const float TURN_POTI_THRESHOLD = 0.25f;
    
    float joyX = in.joyX;  // -1..+1 (nur Left/Right)
    float joyY = in.joyY;  // -1..+1 (Forward/Backward)
    float turn = in.turn;  // -1..+1 vom Turn-Poti (TurnLeft/TurnRight)
    
    // Turn-Poti für Drehen (hat Vorrang)
    if (fabsf(turn) > TURN_POTI_THRESHOLD) {
        if (turn > 0) {
            desiredMove = MoveDir::TurnRight;
        } else {
            desiredMove = MoveDir::TurnLeft;
        }
    }
    // Joystick Y: Forward/Backward
    else if (fabsf(joyY) > DEAD_ZONE) {
        if (joyY > 0) {
            desiredMove = MoveDir::Forward;
        } else {
            desiredMove = MoveDir::Backward;
        }
    }
    // Joystick X: nur Left/Right (kein Turn)
    else if (fabsf(joyX) > DEAD_ZONE) {
        if (joyX > 0) {
            desiredMove = MoveDir::Right;
        } else {
            desiredMove = MoveDir::Left;
        }
    }
    
    // Speed aus speedMax (Poti-Wert, bereits 10..100)
    int desiredSpeed = in.speedMax;
    
    apply(desiredMove, desiredSpeed, speedMin);
}

void DriveControlV3::apply(MoveDir desiredMove, int desiredSpeed, int speedMin) {
    if (!ws || !ws->connected()) return;
    
    // Speed-Änderung senden
    if (desiredSpeed != speed) {
        speed = desiredSpeed;
        ws->sendSetSpeed(speed);
    }
    
    // Walk-Parameter senden wenn geändert
    if (paramsChanged) {
        ws->sendSetWalkParams(walkParams);
        paramsChanged = false;
    }
    
    // Bewegungsstatus ändern
    if (desiredMove != move) {
        if (desiredMove == MoveDir::None) {
            // Stoppen
            if (moving) {
                ws->sendMoveStop();
                moving = false;
            }
        } else {
            // Neue Bewegung starten
            ws->sendMoveStartEx(moveDirToString(desiredMove), nullptr);
            moving = true;
        }
        move = desiredMove;
    }
}

void DriveControlV3::forceStop() {
    if (ws) {
        ws->sendStop();
    }
    move = MoveDir::None;
    moving = false;
}

const char* DriveControlV3::currentMove() const {
    return moveDirToString(move);
}

const char* DriveControlV3::moveDirToString(MoveDir dir) const {
    switch (dir) {
        case MoveDir::Forward:   return "forward";
        case MoveDir::Backward:  return "backward";
        case MoveDir::Left:      return "left";
        case MoveDir::Right:     return "right";
        case MoveDir::TurnLeft:  return "turnleft";
        case MoveDir::TurnRight: return "turnright";
        default:                 return "none";
    }
}

// =============================================================================
// Walk-Parameter Setter
// =============================================================================

void DriveControlV3::setWalkParams(const WalkParams& params) {
    walkParams = params;
    walkParams.validate();
    paramsChanged = true;
}

void DriveControlV3::setStride(float stride) {
    walkParams.stride = stride;
    walkParams.validate();
    if (ws) ws->sendSetStride(walkParams.stride);
}

void DriveControlV3::setSubSteps(uint8_t steps) {
    walkParams.subSteps = steps;
    walkParams.validate();
    if (ws) ws->sendSetSubSteps(walkParams.subSteps);
}

void DriveControlV3::setTimingProfile(TimingProfile profile) {
    walkParams.profile = profile;
    if (ws) ws->sendSetTimingProfile(profile);
}

void DriveControlV3::enableRamp(bool enable) {
    walkParams.rampEnabled = enable;
    if (ws) ws->sendEnableRamp(enable, walkParams.rampCycles);
}

void DriveControlV3::updateStrideFromPoti(int potiValue) {
    // Poti: 0-100 -> Stride: 0.5-1.5
    float stride = 0.5f + (potiValue / 100.0f) * 1.0f;
    if (abs(stride - walkParams.stride) > 0.05f) {
        setStride(stride);
    }
}
