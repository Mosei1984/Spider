// =============================================================================
// WalkParams.h - Erweiterte Walk-Parameter für v3 Protokoll
// =============================================================================
// SpiderRemote-ESP32 v3 Extension
// Definiert Parameter für Stride, Interpolation, Timing, Ramp
// =============================================================================
#pragma once
#include <Arduino.h>

// =============================================================================
// Timing-Profile (muss mit Spider-Controller übereinstimmen)
// =============================================================================
enum class TimingProfile : uint8_t {
    LINEAR = 0,        // Gleichmäßiges Timing
    SWING_STANCE = 1,  // Swing schneller, Stance langsamer
    EASE_IN_OUT = 2    // Sanfter Start/Stop
};

// =============================================================================
// Walk-Parameter Struktur
// =============================================================================
struct WalkParams {
    float stride;           // Stride-Faktor (0.3 - 2.0), Default: 1.0
    uint8_t subSteps;       // Interpolations-Substeps (1-16), Default: 8
    TimingProfile profile;  // Timing-Profil
    float swingMul;         // Swing-Phase Multiplikator (< 1.0 = schneller)
    float stanceMul;        // Stance-Phase Multiplikator (> 1.0 = langsamer)
    bool rampEnabled;       // Soft-Start/Stop aktiviert
    uint8_t rampCycles;     // Anzahl Zyklen für Ramp
    
    // Default-Werte
    WalkParams() 
        : stride(1.0f)
        , subSteps(8)
        , profile(TimingProfile::SWING_STANCE)
        , swingMul(0.75f)
        , stanceMul(1.25f)
        , rampEnabled(false)
        , rampCycles(3) {}
    
    // Validierung und Clamping
    void validate() {
        if (stride < 0.3f) stride = 0.3f;
        if (stride > 2.0f) stride = 2.0f;
        if (subSteps < 1) subSteps = 1;
        if (subSteps > 16) subSteps = 16;
        if (swingMul < 0.3f) swingMul = 0.3f;
        if (swingMul > 2.0f) swingMul = 2.0f;
        if (stanceMul < 0.3f) stanceMul = 0.3f;
        if (stanceMul > 2.0f) stanceMul = 2.0f;
        if (rampCycles > 10) rampCycles = 10;
    }
    
    // Profil-Name für UI
    const char* getProfileName() const {
        switch (profile) {
            case TimingProfile::LINEAR: return "Linear";
            case TimingProfile::SWING_STANCE: return "SwingStance";
            case TimingProfile::EASE_IN_OUT: return "EaseInOut";
            default: return "Unknown";
        }
    }
};

// =============================================================================
// Servo-Kalibrierungs-Parameter
// =============================================================================
struct ServoCalibParams {
    int offset;       // Offset-Korrektur
    int minAngle;     // Minimaler sicherer Winkel
    int maxAngle;     // Maximaler sicherer Winkel
    int centerAngle;  // Neutralpunkt
    
    ServoCalibParams() 
        : offset(0)
        , minAngle(20)
        , maxAngle(160)
        , centerAngle(90) {}
    
    void validate() {
        if (offset < -30) offset = -30;
        if (offset > 30) offset = 30;
        if (minAngle < 0) minAngle = 0;
        if (maxAngle > 180) maxAngle = 180;
        if (centerAngle < minAngle) centerAngle = minAngle;
        if (centerAngle > maxAngle) centerAngle = maxAngle;
    }
};
