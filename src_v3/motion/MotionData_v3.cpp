// =============================================================================
// MotionData_v3.cpp - Motion-Daten Implementierung für v3
// =============================================================================
// Enthält:
//   - Servo-Objekte
//   - PROGMEM Keyframe-Matrizen
//   - Low-level Servo-Control mit Kalibrierungs-Integration
//   - Terrain-Blending
//   - Legacy Blocking Engine
// =============================================================================
#include "MotionData_v3.h"
#include "../calibration/ServoCalibration.h"
#include <Arduino.h>
#include <LittleFS.h>

// =============================================================================
// Servo-Objekte
// =============================================================================
Servo servo_14;  // Index 0 - UR paw
Servo servo_12;  // Index 1 - UR arm
Servo servo_13;  // Index 2 - LR arm
Servo servo_15;  // Index 3 - LR paw
Servo servo_16;  // Index 4 - UL paw
Servo servo_5;   // Index 5 - UL arm
Servo servo_4;   // Index 6 - LL arm
Servo servo_2;   // Index 7 - LL paw

// =============================================================================
// Konstanten
// =============================================================================
const int PWMRES_Min = 1;
const int PWMRES_Max = 180;
const int ALLMATRIX = 9;
const int ALLSERVOS = 8;
const int SERVOMIN = 400;
const int SERVOMAX = 2400;

// =============================================================================
// Globale Variablen
// =============================================================================
int Running_Servo_POS[ALLMATRIX];
int BASEDELAYTIME = 3;
int speedMultiplier = 80;
int Servo_Offset[ALLSERVOS] = { 0, 0, 0, 0, 0, 0, 0, 0 };

// Lift-Sign für Terrain-Offsets
const int8_t liftSign[ALLSERVOS] = { -1, 0, 0, -1, +1, 0, 0, +1 };

// Terrain-Modus
TerrainMode currentTerrainMode = TERRAIN_NORMAL;
int terrainBlendCurrent = 0;
int terrainBlendTarget = 0;
const int TERRAIN_OFFSET = 25;
const int TERRAIN_BLEND_SPEED = 3;

// =============================================================================
// PROGMEM Keyframe-Matrizen (identisch mit Original)
// =============================================================================

const int Servo_Act_0[] PROGMEM = { 90, 90, 90, 90, 90, 90, 90, 90, 500 };
const int Servo_Act_1[] PROGMEM = { 60, 90, 90, 120, 120, 90, 90, 60, 500 };

const int Servo_Prg_1_Step = 2;
const int Servo_Prg_1[][ALLMATRIX] PROGMEM = {
    { 90, 90, 90, 90, 90, 90, 90, 90, 500 },
    { 60, 90, 90, 120, 120, 90, 90, 60, 500 },
};

const int Servo_Prg_2_Step = 11;
const int Servo_Prg_2[][ALLMATRIX] PROGMEM = {
    {  78,  90,  90, 106, 106,  90,  90,  78, 220 },
    {  86,  94,  90, 110, 110,  90,  86,  86, 170 },
    {  90, 108,  90, 110, 110,  90,  72,  90, 140 },
    {  82, 118,  90, 110, 110,  90,  62,  82, 130 },
    {  74, 120,  90,  98,  98,  90,  60,  74, 160 },
    {  72, 102,  90,  90,  90,  90,  78,  72, 220 },
    {  72,  94, 108,  90,  90,  72,  90,  72, 160 },
    {  72,  90, 118, 102, 102,  62,  90,  72, 130 },
    {  86,  90, 120, 110, 110,  60,  90,  86, 140 },
    {  90,  90, 108, 110, 110,  72,  90,  90, 170 },
    {  78,  90,  90, 106, 106,  90,  90,  78, 220 },
};

const int Servo_Prg_3_Step = 11;
const int Servo_Prg_3[][ALLMATRIX] PROGMEM = {
    {  78,  90,  90, 106, 106,  90,  90,  78, 220 },
    {  86,  86,  90, 110, 110,  90,  94,  86, 170 },
    {  90,  72,  90, 110, 110,  90, 108,  90, 140 },
    {  82,  62,  90, 110, 110,  90, 118,  82, 130 },
    {  74,  60,  90,  98,  98,  90, 120,  74, 160 },
    {  72,  78,  90,  90,  90,  90, 102,  72, 220 },
    {  72,  90,  72,  90,  90, 108,  90,  72, 160 },
    {  72,  90,  62, 102, 102, 118,  90,  72, 130 },
    {  86,  90,  60, 110, 110, 120,  90,  86, 140 },
    {  90,  90,  72, 110, 110, 108,  90,  90, 170 },
    {  78,  90,  90, 106, 106,  90,  90,  78, 220 },
};

const int Servo_Prg_4_Step = 11;
const int Servo_Prg_4[][ALLMATRIX] PROGMEM = {
    {  78,  90,  90, 106, 106,  90,  90,  78, 230 },
    {  74,  90,  90,  96,  96,  90,  90,  74, 190 },
    {  72,  90,  72,  96,  96, 108,  90,  72, 160 },
    {  72,  90,  62, 106, 106, 118,  90,  72, 140 },
    {  86,  90,  60, 110, 110, 120,  90,  86, 150 },
    {  90,  90,  78, 110, 110, 102,  90,  90, 220 },
    {  90, 108,  90, 110, 110,  90,  72,  90, 160 },
    {  82, 118,  90, 110, 110,  90,  62,  82, 140 },
    {  74, 120,  90,  98,  98,  90,  60,  74, 150 },
    {  72, 102,  90,  90,  90,  90,  78,  72, 220 },
    {  78,  90,  90, 106, 106,  90,  90,  78, 230 },
};

// Servo_Prg_5 = Gespiegelte Version von Prg_4 (Links→Rechts)
// Swap: [s4,s5,s6,s7] ↔ [s0,s1,s2,s3] + Winkel spiegeln (180-x)
const int Servo_Prg_5_Step = 11;
const int Servo_Prg_5[][ALLMATRIX] PROGMEM = {
    {  74,  90,  90, 102, 102,  90,  90,  74, 230 },
    {  84,  90,  90, 106, 106,  90,  90,  84, 190 },
    {  84,  72,  90, 108, 108,  90, 108,  84, 160 },
    {  74,  62,  90, 108, 108,  90, 118,  74, 140 },
    {  70,  60,  90,  94,  94,  90, 120,  70, 150 },
    {  70,  78,  90,  90,  90,  90, 102,  70, 220 },
    {  70,  90, 108,  90,  90,  72,  90,  70, 160 },
    {  70,  90, 118,  98,  98,  62,  90,  70, 140 },
    {  82,  90, 120, 106, 106,  60,  90,  82, 150 },
    {  90,  90, 102, 108, 108,  78,  90,  90, 220 },
    {  74,  90,  90, 102, 102,  90,  90,  74, 230 },
};

const int Servo_Prg_6_Step = 8;
const int Servo_Prg_6[][ALLMATRIX] PROGMEM = {
    {  78,  90,  90, 106, 106,  90,  90,  78, 220 },
    {  86,  96,  90, 110, 110,  90,  96,  86, 160 },
    {  90, 118,  90, 110, 110,  90, 118,  90, 140 },
    {  82, 132,  90, 110, 110,  90, 132,  82, 130 },
    {  74, 135,  90,  98,  98,  90, 135,  74, 170 },
    {  72, 135, 118,  90,  90, 118, 135,  72, 200 },
    {  72, 118, 132, 106, 106, 132, 118,  72, 160 },
    {  78,  90,  90, 106, 106,  90,  90,  78, 220 },
};

const int Servo_Prg_7_Step = 8;
const int Servo_Prg_7[][ALLMATRIX] PROGMEM = {
    {  78,  90,  90, 106, 106,  90,  90,  78, 220 },
    {  74,  90,  90,  98,  98,  90,  90,  74, 160 },
    {  72,  90,  62,  98,  98,  62,  90,  72, 140 },
    {  72,  90,  50, 110, 110,  50,  90,  72, 130 },
    {  86,  90,  45, 110, 110,  45,  90,  86, 170 },
    {  90,  62,  50, 110, 110,  50,  62,  90, 200 },
    {  82,  50,  62, 110, 110,  62,  50,  82, 160 },
    {  78,  90,  90, 106, 106,  90,  90,  78, 220 },
};

const int Servo_Prg_8_Step = 1;
const int Servo_Prg_8[][ALLMATRIX] PROGMEM = {
    { 110, 90, 90, 70, 70, 90, 90, 110, 500 },
};

const int Servo_Prg_9_Step = 7;
const int Servo_Prg_9[][ALLMATRIX] PROGMEM = {
    { 70, 90, 135, 90, 90, 90, 90, 90, 400 },
    { 170, 90, 135, 90, 90, 90, 90, 90, 400 },
    { 170, 130, 135, 90, 90, 90, 90, 90, 400 },
    { 170, 50, 135, 90, 90, 90, 90, 90, 400 },
    { 170, 130, 135, 90, 90, 90, 90, 90, 400 },
    { 170, 90, 135, 90, 90, 90, 90, 90, 400 },
    { 70, 90, 135, 90, 90, 90, 90, 90, 400 },
};

const int Servo_Prg_10_Step = 11;
const int Servo_Prg_10[][ALLMATRIX] PROGMEM = {
    { 120, 90, 90, 110, 60, 90, 90, 70, 500 },
    { 120, 70, 70, 110, 60, 70, 70, 70, 500 },
    { 120, 110, 110, 110, 60, 110, 110, 70, 500 },
    { 120, 70, 70, 110, 60, 70, 70, 70, 500 },
    { 120, 110, 110, 110, 60, 110, 110, 70, 500 },
    { 70, 90, 90, 70, 110, 90, 90, 110, 500 },
    { 70, 70, 70, 70, 110, 70, 70, 110, 500 },
    { 70, 110, 110, 70, 110, 110, 110, 110, 500 },
    { 70, 70, 70, 70, 110, 70, 70, 110, 500 },
    { 70, 110, 110, 70, 110, 110, 110, 110, 500 },
    { 70, 90, 90, 70, 110, 90, 90, 110, 500 },
};

const int Servo_Prg_11_Step = 7;
const int Servo_Prg_11[][ALLMATRIX] PROGMEM = {
    { 70, 90, 90, 110, 110, 90, 90, 70, 100 },
    { 90, 90, 90, 90, 90, 90, 90, 90, 600 },
    { 70, 90, 90, 110, 110, 90, 90, 70, 500 },
    { 90, 90, 90, 90, 90, 90, 90, 100, 700 },
    { 70, 90, 90, 110, 110, 90, 90, 70, 500 },
    { 90, 90, 90, 90, 90, 90, 90, 100, 800 },
    { 70, 90, 90, 110, 110, 90, 90, 70, 500 },
};

const int Servo_Prg_12_Step = 2;
const int Servo_Prg_12[][ALLMATRIX] PROGMEM = {
    { 30, 90, 90, 150, 150, 90, 90, 30, 500 },
    { 30, 45, 135, 150, 150, 135, 45, 30, 500 },
};

const int Servo_Prg_13_Step = 10;
const int Servo_Prg_13[][ALLMATRIX] PROGMEM = {
    { 90, 90, 90, 90, 90, 90, 90, 90, 400 },
    { 50, 90, 90, 90, 90, 90, 90, 90, 400 },
    { 90, 90, 90, 130, 90, 90, 90, 90, 400 },
    { 90, 90, 90, 90, 90, 90, 90, 50, 400 },
    { 90, 90, 90, 90, 130, 90, 90, 90, 400 },
    { 50, 90, 90, 90, 90, 90, 90, 90, 400 },
    { 90, 90, 90, 130, 90, 90, 90, 90, 400 },
    { 90, 90, 90, 90, 90, 90, 90, 50, 400 },
    { 90, 90, 90, 90, 130, 90, 90, 90, 400 },
    { 90, 90, 90, 90, 90, 90, 90, 90, 400 },
};

const int Servo_Prg_14_Step = 9;
const int Servo_Prg_14[][ALLMATRIX] PROGMEM = {
    { 70, 45, 135, 110, 110, 135, 45, 70, 400 },
    { 115, 45, 135, 65, 110, 135, 45, 70, 400 },
    { 70, 45, 135, 110, 65, 135, 45, 115, 400 },
    { 115, 45, 135, 65, 110, 135, 45, 70, 400 },
    { 70, 45, 135, 110, 65, 135, 45, 115, 400 },
    { 115, 45, 135, 65, 110, 135, 45, 70, 400 },
    { 70, 45, 135, 110, 65, 135, 45, 115, 400 },
    { 115, 45, 135, 65, 110, 135, 45, 70, 400 },
    { 75, 45, 135, 105, 110, 135, 45, 70, 400 },
};

const int Servo_Prg_15_Step = 10;
const int Servo_Prg_15[][ALLMATRIX] PROGMEM = {
    { 70, 45, 45, 110, 110, 135, 135, 70, 400 },
    { 110, 45, 45, 60, 70, 135, 135, 70, 400 },
    { 70, 45, 45, 110, 110, 135, 135, 70, 400 },
    { 110, 45, 45, 110, 70, 135, 135, 120, 400 },
    { 70, 45, 45, 110, 110, 135, 135, 70, 400 },
    { 110, 45, 45, 60, 70, 135, 135, 70, 400 },
    { 70, 45, 45, 110, 110, 135, 135, 70, 400 },
    { 110, 45, 45, 110, 70, 135, 135, 120, 400 },
    { 70, 45, 45, 110, 110, 135, 135, 70, 400 },
    { 70, 90, 90, 110, 110, 90, 90, 70, 400 },
};

// =============================================================================
// Speed-Funktionen
// =============================================================================
void setSpeed(int speed) {
    if (speed < 10) speed = 10;
    if (speed > 250) speed = 250;
    speedMultiplier = speed;
}

int getSpeed() {
    return speedMultiplier;
}

// =============================================================================
// Terrain-Funktionen
// =============================================================================
void setTerrainMode(TerrainMode mode) {
    currentTerrainMode = mode;
    switch (mode) {
        case TERRAIN_UPHILL:
            terrainBlendTarget = TERRAIN_OFFSET;
            break;
        case TERRAIN_DOWNHILL:
            terrainBlendTarget = -TERRAIN_OFFSET;
            break;
        default:
            terrainBlendTarget = 0;
            break;
    }
    Serial.printf("[Motion] Terrain: %s, target=%d\n", getTerrainModeName(), terrainBlendTarget);
}

TerrainMode getTerrainMode() {
    return currentTerrainMode;
}

const char* getTerrainModeName() {
    switch (currentTerrainMode) {
        case TERRAIN_UPHILL: return "uphill";
        case TERRAIN_DOWNHILL: return "downhill";
        default: return "normal";
    }
}

void terrain_blend_tick() {
    if (terrainBlendCurrent < terrainBlendTarget) {
        terrainBlendCurrent += TERRAIN_BLEND_SPEED;
        if (terrainBlendCurrent > terrainBlendTarget) {
            terrainBlendCurrent = terrainBlendTarget;
        }
    } else if (terrainBlendCurrent > terrainBlendTarget) {
        terrainBlendCurrent -= TERRAIN_BLEND_SPEED;
        if (terrainBlendCurrent < terrainBlendTarget) {
            terrainBlendCurrent = terrainBlendTarget;
        }
    }
}

int getTerrainAdjustment(int iServo) {
    if (iServo < 0 || iServo >= ALLSERVOS) return 0;
    if (liftSign[iServo] == 0) return 0;
    return terrainBlendCurrent * liftSign[iServo];
}

// =============================================================================
// Servo-Control (Low-Level) mit Kalibrierungs-Integration
// =============================================================================
void Set_PWM_to_Servo(int iServo, int iValue) {
    if (iServo < 0 || iServo >= ALLSERVOS) return;
    
    // Kalibrierungs-Offset anwenden
    int calibratedValue = ServoCalibration::applyOffset(iServo, iValue);
    
    // Auf Limits clampen
    calibratedValue = ServoCalibration::clampToLimits(iServo, calibratedValue);
    
    // Finale Safety-Clamp auf PWM-Bereich
    if (calibratedValue < PWMRES_Min) calibratedValue = PWMRES_Min;
    if (calibratedValue > PWMRES_Max) calibratedValue = PWMRES_Max;
    
    // Servo ansteuern
    switch (iServo) {
        case 0: servo_14.write(calibratedValue); break;
        case 1: servo_12.write(calibratedValue); break;
        case 2: servo_13.write(calibratedValue); break;
        case 3: servo_15.write(calibratedValue); break;
        case 4: servo_16.write(calibratedValue); break;
        case 5: servo_5.write(calibratedValue); break;
        case 6: servo_4.write(calibratedValue); break;
        case 7: servo_2.write(calibratedValue); break;
    }
}

// =============================================================================
// Legacy Blocking Motion Engine
// =============================================================================
void Servo_PROGRAM_Run(const int iMatrix[][ALLMATRIX], int iSteps) {
    for (int MainLoopIndex = 0; MainLoopIndex < iSteps; MainLoopIndex++) {
        int originalTime = pgm_read_word(&iMatrix[MainLoopIndex][ALLMATRIX - 1]);
        int timePercent = (110 - speedMultiplier) / 3;
        if (timePercent < 5) timePercent = 5;
        int InterTotalTime = (originalTime * timePercent) / 100;
        if (InterTotalTime < BASEDELAYTIME) InterTotalTime = BASEDELAYTIME;
        
        int InterDelayCounter = (InterTotalTime + BASEDELAYTIME - 1) / BASEDELAYTIME;
        if (InterDelayCounter < 1) InterDelayCounter = 1;
        int EffectiveTotalTime = InterDelayCounter * BASEDELAYTIME;

        for (int InterStepLoop = 1; InterStepLoop <= InterDelayCounter; InterStepLoop++) {
            int t = InterStepLoop * BASEDELAYTIME;

            for (int ServoIndex = 0; ServoIndex < ALLSERVOS; ServoIndex++) {
                int start = Running_Servo_POS[ServoIndex];
                int target = pgm_read_word(&iMatrix[MainLoopIndex][ServoIndex]);

                if (start == target) continue;

                int delta = abs(target - start);
                int moved = map(t, 0, EffectiveTotalTime, 0, delta);
                int value = (target > start) ? (start + moved) : (start - moved);
                Set_PWM_to_Servo(ServoIndex, value);
            }

            delay(BASEDELAYTIME);
            yield();
        }

        for (int ServoIndex = 0; ServoIndex < ALLSERVOS; ServoIndex++) {
            int target = pgm_read_word(&iMatrix[MainLoopIndex][ServoIndex]);
            Set_PWM_to_Servo(ServoIndex, target);
        }

        for (int Index = 0; Index < ALLMATRIX; Index++) {
            Running_Servo_POS[Index] = pgm_read_word(&iMatrix[MainLoopIndex][Index]);
        }
    }
}

void Servo_PROGRAM_Zero() {
    for (int Index = 0; Index < ALLMATRIX; Index++) {
        Running_Servo_POS[Index] = pgm_read_word(&Servo_Act_0[Index]);
    }
    for (int iServo = 0; iServo < ALLSERVOS; iServo++) {
        Set_PWM_to_Servo(iServo, Running_Servo_POS[iServo]);
        delay(10);
    }
    for (int Index = 0; Index < ALLMATRIX; Index++) {
        Running_Servo_POS[Index] = pgm_read_word(&Servo_Act_1[Index]);
    }
    for (int iServo = 0; iServo < ALLSERVOS; iServo++) {
        Set_PWM_to_Servo(iServo, Running_Servo_POS[iServo]);
        delay(10);
    }
}

// =============================================================================
// Blockierende Bewegungsfunktionen
// =============================================================================
void standby() {
    Servo_PROGRAM_Run(Servo_Prg_1, Servo_Prg_1_Step);
}

void sleep() {
    Servo_PROGRAM_Run(Servo_Prg_1, Servo_Prg_1_Step);
    Servo_PROGRAM_Run(Servo_Prg_12, Servo_Prg_12_Step);
}

void lie() {
    Servo_PROGRAM_Run(Servo_Prg_8, Servo_Prg_8_Step);
}

void forward_blocking() {
    Servo_PROGRAM_Run(Servo_Prg_2, Servo_Prg_2_Step);
}

void back_blocking() {
    Servo_PROGRAM_Run(Servo_Prg_3, Servo_Prg_3_Step);
}

void turnleft_blocking() {
    Servo_PROGRAM_Run(Servo_Prg_6, Servo_Prg_6_Step);
}

void turnright_blocking() {
    Servo_PROGRAM_Run(Servo_Prg_7, Servo_Prg_7_Step);
}

void leftmove_blocking() {
    Servo_PROGRAM_Run(Servo_Prg_4, Servo_Prg_4_Step);
}

void rightmove_blocking() {
    Servo_PROGRAM_Run(Servo_Prg_5, Servo_Prg_5_Step);
}

void hello() {
    Servo_PROGRAM_Run(Servo_Prg_9, Servo_Prg_9_Step);
    Servo_PROGRAM_Run(Servo_Prg_1, Servo_Prg_1_Step);
}

void dance1() {
    Servo_PROGRAM_Run(Servo_Prg_13, Servo_Prg_13_Step);
}

void dance2() {
    Servo_PROGRAM_Run(Servo_Prg_14, Servo_Prg_14_Step);
}

void dance3() {
    Servo_PROGRAM_Run(Servo_Prg_15, Servo_Prg_15_Step);
}

void pushup() {
    Servo_PROGRAM_Run(Servo_Prg_11, Servo_Prg_11_Step);
}

void fighting() {
    Servo_PROGRAM_Run(Servo_Prg_10, Servo_Prg_10_Step);
}

void calibpose() {
    const int CALIB_POSE[8] = {135, 45, 135, 45, 45, 135, 45, 135};
    Serial.println(F("[Motion] Calibration pose"));
    for (int i = 0; i < ALLSERVOS; i++) {
        Set_PWM_to_Servo(i, CALIB_POSE[i]);
        Running_Servo_POS[i] = CALIB_POSE[i];
    }
}
