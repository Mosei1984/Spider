// =============================================================================
// GaitRuntime.cpp - Implementierung der erweiterten Motion Engine
// =============================================================================
#include "GaitRuntime.h"
#include <LittleFS.h>

// =============================================================================
// Globale Instanzen
// =============================================================================
GaitRuntimeConfig gaitConfig;
static GaitMotionState gaitState;

// Externe Abhängigkeiten (aus MotionData)
extern int Running_Servo_POS[];
extern int speedMultiplier;
extern int Servo_Offset[];
extern void Set_PWM_to_Servo(int iServo, int iValue);
extern void terrain_blend_tick();
extern int getTerrainAdjustment(int iServo);

// =============================================================================
// Namespace: GaitRuntimeInternal - Helper-Funktionen
// =============================================================================
namespace GaitRuntimeInternal {

// Smoothstep Easing: f(t) = t² * (3 - 2t)
float smoothstep(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

// Phase-Erkennung basierend auf Knee/Paw-Servo-Deltas
GaitPhase detectPhase(const int fromPose[], const int toPose[]) {
    int liftCount = 0;
    
    // Nur Knee-Servos (Paws) für Lift-Erkennung prüfen
    for (uint8_t i = 0; i < KNEE_COUNT; i++) {
        uint8_t idx = KNEE_SERVO_IDX[i];
        int delta = toPose[idx] - fromPose[idx];
        
        // Lift-Sign berücksichtigen: delta * liftSign > 0 = Bein hebt
        if (LIFT_SIGN[idx] != 0) {
            int signedDelta = delta * LIFT_SIGN[idx];
            if (signedDelta > gaitConfig.timing.liftThreshold) {
                liftCount++;
            }
        }
    }
    
    // Mindestens 1 Bein hebt -> Swing-Phase
    if (liftCount >= 1) {
        return GaitPhase::SWING;
    }
    return GaitPhase::STANCE;
}

// Stride-Skalierung für einen Servo-Winkel
int applyStrideScale(int rawAngle, uint8_t servoIdx, float effectiveStride) {
    if (!gaitConfig.stride.enabled || servoIdx >= SERVO_COUNT) {
        return rawAngle;
    }
    
    int center = gaitConfig.servoLimits[servoIdx].centerAngle;
    float scale = 1.0f;
    
    // Hip oder Knee?
    bool isHip = false;
    for (uint8_t i = 0; i < HIP_COUNT; i++) {
        if (HIP_SERVO_IDX[i] == servoIdx) {
            isHip = true;
            break;
        }
    }
    
    if (isHip) {
        // Hip-Servos: volle Stride-Skalierung
        scale = effectiveStride;
    } else {
        // Knee-Servos: moderatere Skalierung
        // LiftFactor = 1.0 + kneeMix * (strideFactor - 1.0)
        scale = 1.0f + gaitConfig.stride.kneeMix * (effectiveStride - 1.0f);
    }
    
    // Skalierung um Neutralpunkt
    float delta = (float)(rawAngle - center);
    float scaled = center + delta * scale;
    
    return (int)(scaled + 0.5f);  // Runden
}

// Clamp auf konfigurierte Servo-Limits
int clampToLimits(int angle, uint8_t servoIdx) {
    if (servoIdx >= SERVO_COUNT) return angle;
    
    const ServoLimits& limits = gaitConfig.servoLimits[servoIdx];
    if (angle < limits.minAngle) return limits.minAngle;
    if (angle > limits.maxAngle) return limits.maxAngle;
    return angle;
}

} // namespace GaitRuntimeInternal

// =============================================================================
// Namespace: GaitRuntime - Haupt-API
// =============================================================================
namespace GaitRuntime {

void init() {
    gaitState = GaitMotionState();
    gaitConfig = GaitRuntimeConfig();
    gaitConfig.validate();
    
    // Versuche gespeicherte Konfiguration zu laden
    loadConfig();
    
    Serial.println(F("[GaitRuntime] Initialisiert"));
}

void start(const int matrix[][9], int steps) {
    if (steps <= 0 || matrix == nullptr) return;
    
    gaitState.matrix = matrix;
    gaitState.totalSteps = steps;
    gaitState.currentStep = 0;
    gaitState.sequenceComplete = false;
    gaitState.isFirstCycle = (gaitState.cycleCount == 0);
    
    // Ramp: Stride von current zu target über Zyklen interpolieren
    if (gaitConfig.ramp.enabled && gaitState.isFirstCycle) {
        gaitConfig.ramp.currentStride = 0.3f;  // Start klein
    }
    
    // Aktuelle Servo-Positionen als Startpunkt
    for (int i = 0; i < SERVO_COUNT; i++) {
        gaitState.fromPose[i] = Running_Servo_POS[i];
        gaitState.toPose[i] = pgm_read_word(&matrix[0][i]);
    }
    
    // Effektiven Stride berechnen (mit Ramp)
    float effectiveStride = gaitConfig.ramp.enabled 
        ? gaitConfig.ramp.currentStride 
        : gaitConfig.stride.strideFactor;
    
    // Stride-Skalierung auf Zielpose anwenden
    for (int i = 0; i < SERVO_COUNT; i++) {
        int scaled = GaitRuntimeInternal::applyStrideScale(
            gaitState.toPose[i], i, effectiveStride);
        gaitState.scaledToPose[i] = GaitRuntimeInternal::clampToLimits(scaled, i);
    }
    
    // Phase erkennen für Timing-Shaping
    gaitState.currentPhase = GaitRuntimeInternal::detectPhase(
        gaitState.fromPose, gaitState.scaledToPose);
    
    // Basis-Timing aus Keyframe
    int originalTime = pgm_read_word(&matrix[0][8]);
    int timePercent = (110 - speedMultiplier) / 3;
    if (timePercent < 5) timePercent = 5;
    gaitState.segmentDuration = (originalTime * timePercent) / 100;
    if (gaitState.segmentDuration < 20) gaitState.segmentDuration = 20;
    
    // Timing-Shaping anwenden
    float timingMult = 1.0f;
    if (gaitConfig.timing.profile == TimingProfile::SWING_STANCE) {
        timingMult = (gaitState.currentPhase == GaitPhase::SWING)
            ? gaitConfig.timing.swingMultiplier
            : gaitConfig.timing.stanceMultiplier;
    }
    gaitState.adjustedDuration = (int)(gaitState.segmentDuration * timingMult);
    if (gaitState.adjustedDuration < 15) gaitState.adjustedDuration = 15;
    
    gaitState.segmentStartMs = millis();
    gaitState.active = true;
    
    Serial.printf("[GaitRuntime] Start: %d steps, stride=%.2f, phase=%s\n", 
        steps, effectiveStride,
        gaitState.currentPhase == GaitPhase::SWING ? "SWING" : "STANCE");
}

bool tick(unsigned long nowMs) {
    if (!gaitState.active) return false;
    
    // Terrain-Blending ticken
    terrain_blend_tick();
    
    unsigned long elapsed = nowMs - gaitState.segmentStartMs;
    
    // Interpolationsfortschritt berechnen
    float alpha = (float)elapsed / (float)gaitState.adjustedDuration;
    if (alpha > 1.0f) alpha = 1.0f;
    
    // Easing anwenden
    float eased = gaitConfig.interpolation.smoothstepEnabled 
        ? GaitRuntimeInternal::smoothstep(alpha)
        : alpha;
    
    // Alle Servos interpolieren
    for (int i = 0; i < SERVO_COUNT; i++) {
        int start = gaitState.fromPose[i];
        int target = gaitState.scaledToPose[i];
        int value = start + (int)((target - start) * eased);
        
        // Terrain-Offset hinzufügen
        value += getTerrainAdjustment(i);
        
        // Final clampen und setzen
        value = GaitRuntimeInternal::clampToLimits(value, i);
        Set_PWM_to_Servo(i, value);
    }
    
    // Segment abgeschlossen?
    if (elapsed >= (unsigned long)gaitState.adjustedDuration) {
        // Exakte Endposition setzen
        for (int i = 0; i < SERVO_COUNT; i++) {
            int finalValue = gaitState.scaledToPose[i] + getTerrainAdjustment(i);
            finalValue = GaitRuntimeInternal::clampToLimits(finalValue, i);
            Set_PWM_to_Servo(i, finalValue);
            Running_Servo_POS[i] = gaitState.scaledToPose[i];
        }
        
        // Nächster Step
        gaitState.currentStep++;
        
        if (gaitState.currentStep >= gaitState.totalSteps) {
            // Sequenz beendet
            gaitState.active = false;
            gaitState.sequenceComplete = true;
            gaitState.cycleCount++;
            
            // Ramp-Update am Zyklusende
            if (gaitConfig.ramp.enabled && gaitConfig.ramp.rampCycles > 0) {
                float delta = gaitConfig.ramp.targetStride - gaitConfig.ramp.currentStride;
                gaitConfig.ramp.currentStride += delta / gaitConfig.ramp.rampCycles;
                
                // Ramp beenden wenn Ziel erreicht
                if (gaitState.cycleCount >= gaitConfig.ramp.rampCycles) {
                    gaitConfig.ramp.currentStride = gaitConfig.ramp.targetStride;
                }
            }
            
            Serial.println(F("[GaitRuntime] Sequenz beendet"));
            return false;
        }
        
        // Nächstes Segment vorbereiten
        for (int i = 0; i < SERVO_COUNT; i++) {
            gaitState.fromPose[i] = Running_Servo_POS[i];
            gaitState.toPose[i] = pgm_read_word(&gaitState.matrix[gaitState.currentStep][i]);
        }
        
        // Effektiven Stride berechnen
        float effectiveStride = gaitConfig.ramp.enabled 
            ? gaitConfig.ramp.currentStride 
            : gaitConfig.stride.strideFactor;
        
        // Stride-Skalierung
        for (int i = 0; i < SERVO_COUNT; i++) {
            int scaled = GaitRuntimeInternal::applyStrideScale(
                gaitState.toPose[i], i, effectiveStride);
            gaitState.scaledToPose[i] = GaitRuntimeInternal::clampToLimits(scaled, i);
        }
        
        // Phase erkennen
        gaitState.currentPhase = GaitRuntimeInternal::detectPhase(
            gaitState.fromPose, gaitState.scaledToPose);
        
        // Timing
        int originalTime = pgm_read_word(&gaitState.matrix[gaitState.currentStep][8]);
        int timePercent = (110 - speedMultiplier) / 3;
        if (timePercent < 5) timePercent = 5;
        gaitState.segmentDuration = (originalTime * timePercent) / 100;
        if (gaitState.segmentDuration < 20) gaitState.segmentDuration = 20;
        
        // Timing-Shaping
        float timingMult = 1.0f;
        if (gaitConfig.timing.profile == TimingProfile::SWING_STANCE) {
            timingMult = (gaitState.currentPhase == GaitPhase::SWING)
                ? gaitConfig.timing.swingMultiplier
                : gaitConfig.timing.stanceMultiplier;
        }
        gaitState.adjustedDuration = (int)(gaitState.segmentDuration * timingMult);
        if (gaitState.adjustedDuration < 15) gaitState.adjustedDuration = 15;
        
        gaitState.segmentStartMs = nowMs;
    }
    
    return true;
}

void stop() {
    gaitState.active = false;
    gaitState.sequenceComplete = true;
    gaitState.cycleCount = 0;
    gaitState.isFirstCycle = true;
    
    // Ramp zurücksetzen
    gaitConfig.ramp.currentStride = gaitConfig.stride.strideFactor;
}

bool isActive() {
    return gaitState.active;
}

bool isSequenceComplete() {
    return gaitState.sequenceComplete;
}

void resetSequenceFlag() {
    gaitState.sequenceComplete = false;
}

const GaitMotionState& getState() {
    return gaitState;
}

// =============================================================================
// Konfigurations-Setter
// =============================================================================
void setStrideFactor(float factor) {
    gaitConfig.stride.strideFactor = factor;
    gaitConfig.validate();
    Serial.printf("[GaitRuntime] StrideFactor: %.2f\n", gaitConfig.stride.strideFactor);
}

void setSubSteps(uint8_t steps) {
    gaitConfig.interpolation.subSteps = steps;
    gaitConfig.validate();
    Serial.printf("[GaitRuntime] SubSteps: %d\n", gaitConfig.interpolation.subSteps);
}

void setTimingProfile(TimingProfile profile) {
    gaitConfig.timing.profile = profile;
    Serial.printf("[GaitRuntime] TimingProfile: %d\n", (int)profile);
}

void setSwingMultiplier(float mult) {
    gaitConfig.timing.swingMultiplier = mult;
    gaitConfig.validate();
}

void setStanceMultiplier(float mult) {
    gaitConfig.timing.stanceMultiplier = mult;
    gaitConfig.validate();
}

void enableRamp(bool enable, uint8_t cycles) {
    gaitConfig.ramp.enabled = enable;
    gaitConfig.ramp.rampCycles = cycles;
    gaitConfig.validate();
    
    if (enable) {
        gaitState.cycleCount = 0;
        gaitState.isFirstCycle = true;
    }
    Serial.printf("[GaitRuntime] Ramp: %s, cycles=%d\n", enable ? "ON" : "OFF", cycles);
}

void setServoLimits(uint8_t servo, int minAngle, int maxAngle, int centerAngle) {
    if (servo >= SERVO_COUNT) return;
    
    gaitConfig.servoLimits[servo].minAngle = minAngle;
    gaitConfig.servoLimits[servo].maxAngle = maxAngle;
    gaitConfig.servoLimits[servo].centerAngle = centerAngle;
    
    Serial.printf("[GaitRuntime] Servo %d limits: %d-%d, center=%d\n", 
        servo, minAngle, maxAngle, centerAngle);
}

void setTargetStride(float target) {
    gaitConfig.ramp.targetStride = target;
    if (target < 0.3f) gaitConfig.ramp.targetStride = 0.3f;
    if (target > 2.0f) gaitConfig.ramp.targetStride = 2.0f;
}

// =============================================================================
// Persistenz (LittleFS)
// =============================================================================
static const char* CONFIG_FILE = "/gait_config.dat";

bool saveConfig() {
    File f = LittleFS.open(CONFIG_FILE, "w");
    if (!f) {
        Serial.println(F("[GaitRuntime] Config save failed"));
        return false;
    }
    
    // Header für Versionscheck
    uint8_t version = 1;
    f.write(&version, 1);
    
    // Stride
    f.write((uint8_t*)&gaitConfig.stride, sizeof(StrideConfig));
    
    // Timing
    f.write((uint8_t*)&gaitConfig.timing, sizeof(TimingConfig));
    
    // Interpolation
    f.write((uint8_t*)&gaitConfig.interpolation, sizeof(InterpolationConfig));
    
    // Servo-Limits
    for (uint8_t i = 0; i < SERVO_COUNT; i++) {
        f.write((uint8_t*)&gaitConfig.servoLimits[i], sizeof(ServoLimits));
    }
    
    f.close();
    Serial.println(F("[GaitRuntime] Config saved"));
    return true;
}

bool loadConfig() {
    File f = LittleFS.open(CONFIG_FILE, "r");
    if (!f) {
        Serial.println(F("[GaitRuntime] No config file, using defaults"));
        return false;
    }
    
    // Version prüfen
    uint8_t version = 0;
    f.read(&version, 1);
    if (version != 1) {
        Serial.println(F("[GaitRuntime] Config version mismatch"));
        f.close();
        return false;
    }
    
    // Stride
    f.read((uint8_t*)&gaitConfig.stride, sizeof(StrideConfig));
    
    // Timing
    f.read((uint8_t*)&gaitConfig.timing, sizeof(TimingConfig));
    
    // Interpolation
    f.read((uint8_t*)&gaitConfig.interpolation, sizeof(InterpolationConfig));
    
    // Servo-Limits
    for (uint8_t i = 0; i < SERVO_COUNT; i++) {
        f.read((uint8_t*)&gaitConfig.servoLimits[i], sizeof(ServoLimits));
    }
    
    f.close();
    gaitConfig.validate();
    Serial.println(F("[GaitRuntime] Config loaded"));
    return true;
}

} // namespace GaitRuntime
