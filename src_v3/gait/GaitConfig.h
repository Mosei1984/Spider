// =============================================================================
// GaitConfig.h - Zentrale Gait-Konfiguration für Stride, Timing, Servo-Limits
// =============================================================================
// v3 Gait Runtime Module für ESP8266 Spider Controller
// Autor: Amp (Orchestrator-Agent)
// =============================================================================
#ifndef GAIT_CONFIG_H
#define GAIT_CONFIG_H

#include <Arduino.h>

// =============================================================================
// Servo-Konfiguration
// =============================================================================
static const uint8_t SERVO_COUNT = 8;

// Servo-Typ-Indizes (aus Codebase-Analyse bestätigt)
// Hip (Arm) Servos: stärkere Stride-Skalierung
static const uint8_t HIP_SERVO_IDX[] = { 1, 2, 5, 6 };
static const uint8_t HIP_COUNT = 4;

// Knee (Paw) Servos: moderatere Stride-Skalierung
static const uint8_t KNEE_SERVO_IDX[] = { 0, 3, 4, 7 };
static const uint8_t KNEE_COUNT = 4;

// Lift-Sign für Swing/Stance Erkennung (aus MotionData.cpp)
// +1 = positiver Delta = Bein hebt, -1 = negativer Delta = Bein hebt
static const int8_t LIFT_SIGN[SERVO_COUNT] = { -1, 0, 0, -1, +1, 0, 0, +1 };

// =============================================================================
// Stride-Skalierung Konfiguration
// =============================================================================
struct StrideConfig {
    float strideFactor;      // Gesamtskalierung (1.0 = 100%)
    float kneeMix;           // Knee-Dämpfung: 0.0 = keine Skalierung, 1.0 = volle Skalierung
    bool enabled;            // Stride-Skalierung aktiv
    
    // Defaults aus User-Anforderung
    StrideConfig() : strideFactor(1.0f), kneeMix(0.5f), enabled(true) {}
};

// =============================================================================
// Timing-Profile Konfiguration
// =============================================================================
enum class TimingProfile : uint8_t {
    LINEAR = 0,        // Gleichmäßiges Timing
    SWING_STANCE = 1,  // Swing schneller, Stance langsamer
    EASE_IN_OUT = 2    // Sanfter Start/Stop
};

struct TimingConfig {
    TimingProfile profile;
    float swingMultiplier;   // dt-Faktor für Swing-Phase (< 1.0 = schneller)
    float stanceMultiplier;  // dt-Faktor für Stance-Phase (> 1.0 = langsamer)
    int liftThreshold;       // Mindest-Delta für Swing-Erkennung (Grad)
    
    // Defaults aus User-Anforderung
    TimingConfig() 
        : profile(TimingProfile::SWING_STANCE)
        , swingMultiplier(0.75f)
        , stanceMultiplier(1.25f)
        , liftThreshold(8) {}
};

// =============================================================================
// Interpolation / Micro-Stepping Konfiguration
// =============================================================================
struct InterpolationConfig {
    uint8_t subSteps;        // Substeps pro Keyframe-Übergang (1-16)
    bool smoothstepEnabled;  // Smoothstep Easing aktiviert
    
    // Defaults aus User-Anforderung
    InterpolationConfig() : subSteps(8), smoothstepEnabled(true) {}
};

// =============================================================================
// Soft-Start/Stop Ramp Konfiguration
// =============================================================================
struct RampConfig {
    bool enabled;
    uint8_t rampCycles;      // Anzahl Zyklen für Ramp (0 = instant)
    float currentStride;     // Aktueller interpolierter Stride
    float targetStride;      // Ziel-Stride
    
    RampConfig() : enabled(false), rampCycles(3), currentStride(1.0f), targetStride(1.0f) {}
};

// =============================================================================
// Servo-Limits pro Servo (kalibrierbar)
// =============================================================================
struct ServoLimits {
    int minAngle;            // Minimaler sicherer Winkel
    int maxAngle;            // Maximaler sicherer Winkel
    int centerAngle;         // Neutralpunkt für Stride-Skalierung
    
    ServoLimits() : minAngle(20), maxAngle(160), centerAngle(90) {}
    ServoLimits(int min, int max, int center) : minAngle(min), maxAngle(max), centerAngle(center) {}
};

// =============================================================================
// Gesamte Gait-Runtime Konfiguration
// =============================================================================
struct GaitRuntimeConfig {
    StrideConfig stride;
    TimingConfig timing;
    InterpolationConfig interpolation;
    RampConfig ramp;
    ServoLimits servoLimits[SERVO_COUNT];
    
    // Initialisierung mit Standard-Werten
    GaitRuntimeConfig() {
        // Standard-Limits für alle Servos (konservativ)
        for (uint8_t i = 0; i < SERVO_COUNT; i++) {
            servoLimits[i] = ServoLimits(20, 160, 90);
        }
    }
    
    // Validierung und Clamping
    void validate() {
        // Stride-Factor begrenzen
        if (stride.strideFactor < 0.3f) stride.strideFactor = 0.3f;
        if (stride.strideFactor > 2.0f) stride.strideFactor = 2.0f;
        
        // KneeMix begrenzen
        if (stride.kneeMix < 0.0f) stride.kneeMix = 0.0f;
        if (stride.kneeMix > 1.0f) stride.kneeMix = 1.0f;
        
        // SubSteps begrenzen
        if (interpolation.subSteps < 1) interpolation.subSteps = 1;
        if (interpolation.subSteps > 16) interpolation.subSteps = 16;
        
        // Timing-Multiplier begrenzen
        if (timing.swingMultiplier < 0.3f) timing.swingMultiplier = 0.3f;
        if (timing.swingMultiplier > 2.0f) timing.swingMultiplier = 2.0f;
        if (timing.stanceMultiplier < 0.3f) timing.stanceMultiplier = 0.3f;
        if (timing.stanceMultiplier > 2.0f) timing.stanceMultiplier = 2.0f;
        
        // Ramp-Cycles begrenzen
        if (ramp.rampCycles > 10) ramp.rampCycles = 10;
    }
};

// =============================================================================
// Globale Gait-Konfiguration (extern deklariert, in GaitRuntime.cpp definiert)
// =============================================================================
extern GaitRuntimeConfig gaitConfig;

#endif // GAIT_CONFIG_H
