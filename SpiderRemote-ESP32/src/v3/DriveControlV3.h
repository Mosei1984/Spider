// =============================================================================
// DriveControlV3.h - Erweiterter Drive Controller für v3
// =============================================================================
// SpiderRemote-ESP32 v3 Extension
// Erweitert DriveControl um Walk-Parameter Unterstützung
// =============================================================================
#pragma once
#include <Arduino.h>
#include "WsClientV3.h"
#include "../Inputs.h"
#include "WalkParams.h"

class DriveControlV3 {
public:
    void begin(WsClientV3* ws);
    void tick(const InputState& in, int speedMin);
    
    const char* currentMove() const;
    int currentSpeed() const { return speed; }
    
    void forceStop();
    
    // Walk-Parameter
    void setWalkParams(const WalkParams& params);
    const WalkParams& getWalkParams() const { return walkParams; }
    
    // Einzelne Parameter live ändern
    void setStride(float stride);
    void setSubSteps(uint8_t steps);
    void setTimingProfile(TimingProfile profile);
    void enableRamp(bool enable);
    
    // Stride-Steuerung über Poti (0-100 -> 0.5-1.5)
    void updateStrideFromPoti(int potiValue);
    
private:
    WsClientV3* ws = nullptr;
    
    enum class MoveDir : uint8_t {
        None = 0,
        Forward,
        Backward,
        Left,
        Right,
        TurnLeft,
        TurnRight
    };
    
    MoveDir move = MoveDir::None;
    int speed = -1;
    bool moving = false;
    
    WalkParams walkParams;
    bool paramsChanged = false;
    
    void apply(MoveDir desiredMove, int desiredSpeed, int speedMin);
    const char* moveDirToString(MoveDir dir) const;
};
