// =============================================================================
// GaitRuntime.h - Erweiterte nicht-blockierende Motion Engine mit Stride/Timing
// =============================================================================
// v3 Gait Runtime Module für ESP8266 Spider Controller
// Features:
//   - Stride-Skalierung (Hip stärker, Knee moderater)
//   - Micro-Stepping / feinere Interpolation
//   - Timing-Shaping (Swing schneller, Stance langsamer)
//   - Soft-Start/Stop Ramping
// =============================================================================
#ifndef GAIT_RUNTIME_H
#define GAIT_RUNTIME_H

#include <Arduino.h>
#include "GaitConfig.h"

// =============================================================================
// Phase-Erkennung für Timing-Shaping
// =============================================================================
enum class GaitPhase : uint8_t {
    UNKNOWN = 0,
    SWING,      // Bein in der Luft -> schneller
    STANCE      // Bein am Boden -> langsamer für Stabilität
};

// =============================================================================
// Erweiterter Motion State
// =============================================================================
struct GaitMotionState {
    // Basis-State (kompatibel mit MotionState)
    volatile bool active;
    unsigned long segmentStartMs;
    int segmentDuration;           // Basis-Duration aus Keyframe
    int adjustedDuration;          // Nach Timing-Shaping angepasst
    int currentStep;
    int totalSteps;
    int fromPose[8];
    int toPose[8];
    int scaledToPose[8];           // Nach Stride-Skalierung
    const int (*matrix)[9];
    bool sequenceComplete;
    
    // Erweiterte State-Felder
    GaitPhase currentPhase;
    int cycleCount;                // Für Ramp-Berechnung
    bool isFirstCycle;
    
    GaitMotionState() {
        active = false;
        segmentStartMs = 0;
        segmentDuration = 0;
        adjustedDuration = 0;
        currentStep = 0;
        totalSteps = 0;
        matrix = nullptr;
        sequenceComplete = false;
        currentPhase = GaitPhase::UNKNOWN;
        cycleCount = 0;
        isFirstCycle = true;
        for (int i = 0; i < 8; i++) {
            fromPose[i] = 90;
            toPose[i] = 90;
            scaledToPose[i] = 90;
        }
    }
};

// =============================================================================
// Gait Runtime Engine API
// =============================================================================
namespace GaitRuntime {

// Initialisierung
void init();

// Motion starten (erweitert)
void start(const int matrix[][9], int steps);

// Motion Tick - Rückgabe: true = läuft noch
bool tick(unsigned long nowMs);

// Motion stoppen
void stop();

// Status-Abfragen
bool isActive();
bool isSequenceComplete();
void resetSequenceFlag();

// Aktuellen State lesen (für Debug/UI)
const GaitMotionState& getState();

// Konfiguration zur Laufzeit ändern
void setStrideFactor(float factor);
void setSubSteps(uint8_t steps);
void setTimingProfile(TimingProfile profile);
void setSwingMultiplier(float mult);
void setStanceMultiplier(float mult);
void enableRamp(bool enable, uint8_t cycles = 3);

// Servo-Limits pro Servo setzen (für Kalibrierung)
void setServoLimits(uint8_t servo, int minAngle, int maxAngle, int centerAngle);

// Stride-Ziel setzen (für Ramp)
void setTargetStride(float target);

// Konfiguration laden/speichern (LittleFS)
bool saveConfig();
bool loadConfig();

} // namespace GaitRuntime

// =============================================================================
// Interne Helper (für Tests zugänglich)
// =============================================================================
namespace GaitRuntimeInternal {

// Phase-Erkennung basierend auf Lift-Heuristik
GaitPhase detectPhase(const int fromPose[], const int toPose[]);

// Stride-Skalierung anwenden
int applyStrideScale(int rawAngle, uint8_t servoIdx, float effectiveStride);

// Smoothstep Easing
float smoothstep(float t);

// Clamp auf Servo-Limits
int clampToLimits(int angle, uint8_t servoIdx);

} // namespace GaitRuntimeInternal

#endif // GAIT_RUNTIME_H
