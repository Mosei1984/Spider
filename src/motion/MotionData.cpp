#include "MotionData.h"
#include <Arduino.h>
#include <LittleFS.h>

Servo servo_14;
Servo servo_12;
Servo servo_13;
Servo servo_15;
Servo servo_16;
Servo servo_5;
Servo servo_4;
Servo servo_2;

const int PWMRES_Min = 1;
const int PWMRES_Max = 180;
const int ALLMATRIX = 9;
const int ALLSERVOS = 8;
const int SERVOMIN = 400;
const int SERVOMAX = 2400;

int Running_Servo_POS[ALLMATRIX];
int BASEDELAYTIME = 3;
int speedMultiplier = 80;
int Servo_Offset[ALLSERVOS] = { 0, 0, 0, 0, 0, 0, 0, 0 };

// Lift-Sign für Terrain-Offsets (Delta-Vorzeichen, nicht absolute Inversion!)
// +1 = positiver Offset hebt das Bein (größerer Winkel = Bein hoch)
// -1 = negativer Offset hebt das Bein (kleinerer Winkel = Bein hoch)
// Rechte Seite: kleinerer Winkel = Bein hebt -> liftSign = -1
// Linke Seite: größerer Winkel = Bein hebt -> liftSign = +1
const int8_t liftSign[ALLSERVOS] = { -1, 0, 0, -1, +1, 0, 0, +1 };
// Index: 0=UR(paw), 1=UR(arm), 2=LR(arm), 3=LR(paw), 4=UL(paw), 5=UL(arm), 6=LL(arm), 7=LL(paw)

// Terrain-Modus
TerrainMode currentTerrainMode = TERRAIN_NORMAL;
int terrainBlendCurrent = 0;
int terrainBlendTarget = 0;
const int TERRAIN_OFFSET = 15;  // Max Offset in Grad
const int TERRAIN_BLEND_SPEED = 2;  // Grad pro Tick

// Motion State für nicht-blockierende Engine
MotionState motionState = {
    false,  // active
    0,      // segmentStartMs
    0,      // segmentDuration
    0,      // currentStep
    0,      // totalSteps
    {90,90,90,90,90,90,90,90},  // fromPose
    {90,90,90,90,90,90,90,90},  // toPose
    nullptr, // matrix
    false   // sequenceComplete
};

// ============ Bewegungsmatrizen ============

const int Servo_Act_0[] PROGMEM = { 90, 90, 90, 90, 90, 90, 90, 90, 500 };
const int Servo_Act_1[] PROGMEM = { 60, 90, 90, 120, 120, 90, 90, 60, 500 };

const int Servo_Prg_1_Step = 2;
const int Servo_Prg_1[][ALLMATRIX] PROGMEM = {
    { 90, 90, 90, 90, 90, 90, 90, 90, 500 },
    { 60, 90, 90, 120, 120, 90, 90, 60, 500 },
};

const int Servo_Prg_2_Step = 11;
const int Servo_Prg_2[][ALLMATRIX] PROGMEM = {
    { 70, 90, 90, 110, 110, 90, 90, 70, 200 },
    { 90, 90, 90, 110, 110, 90, 90, 90, 200 },
    { 90, 120, 90, 110, 110, 90, 60, 90, 200 },
    { 70, 120, 90, 110, 110, 90, 60, 70, 200 },
    { 70, 120, 90, 90, 90, 90, 60, 70, 200 },
    { 70, 90, 90, 90, 90, 90, 90, 70, 200 },
    { 70, 90, 120, 90, 90, 60, 90, 70, 200 },
    { 70, 90, 120, 110, 110, 60, 90, 70, 200 },
    { 90, 90, 120, 110, 110, 60, 90, 90, 200 },
    { 90, 90, 90, 110, 110, 90, 90, 90, 200 },
    { 70, 90, 90, 110, 110, 90, 90, 70, 200 },
};

const int Servo_Prg_3_Step = 11;
const int Servo_Prg_3[][ALLMATRIX] PROGMEM = {
    { 70, 90, 90, 110, 110, 90, 90, 70, 200 },
    { 90, 90, 90, 110, 110, 90, 90, 90, 200 },
    { 90, 60, 90, 110, 110, 90, 120, 90, 200 },
    { 70, 60, 90, 110, 110, 90, 120, 70, 200 },
    { 70, 60, 90, 90, 90, 90, 120, 70, 200 },
    { 70, 90, 90, 90, 90, 90, 90, 70, 200 },
    { 70, 90, 60, 90, 90, 120, 90, 70, 200 },
    { 70, 90, 60, 110, 110, 120, 90, 70, 200 },
    { 90, 90, 60, 110, 110, 120, 90, 90, 200 },
    { 90, 90, 90, 110, 110, 90, 90, 90, 200 },
    { 70, 90, 90, 110, 110, 90, 90, 70, 200 },
};

const int Servo_Prg_4_Step = 11;
const int Servo_Prg_4[][ALLMATRIX] PROGMEM = {
    { 70, 90, 90, 110, 110, 90, 90, 70, 200 },
    { 70, 90, 90, 90, 90, 90, 90, 70, 200 },
    { 70, 90, 60, 90, 90, 120, 90, 70, 200 },
    { 70, 90, 60, 110, 110, 120, 90, 70, 200 },
    { 90, 90, 60, 110, 110, 120, 90, 90, 200 },
    { 90, 90, 90, 110, 110, 90, 90, 90, 200 },
    { 90, 120, 90, 110, 110, 90, 60, 90, 200 },
    { 70, 120, 90, 110, 110, 90, 60, 70, 200 },
    { 70, 120, 90, 90, 90, 90, 60, 70, 200 },
    { 70, 90, 90, 90, 90, 90, 90, 70, 200 },
    { 70, 90, 90, 110, 110, 90, 90, 70, 200 },
};

const int Servo_Prg_5_Step = 11;
const int Servo_Prg_5[][ALLMATRIX] PROGMEM = {
    { 70, 90, 90, 110, 110, 90, 90, 70, 200 },
    { 90, 90, 90, 110, 110, 90, 90, 90, 200 },
    { 90, 60, 90, 110, 110, 90, 120, 90, 200 },
    { 70, 60, 90, 110, 110, 90, 120, 70, 200 },
    { 70, 60, 90, 90, 90, 90, 120, 70, 200 },
    { 70, 90, 90, 90, 90, 90, 90, 70, 200 },
    { 70, 90, 120, 90, 90, 60, 90, 70, 200 },
    { 70, 90, 120, 110, 110, 60, 90, 70, 200 },
    { 90, 90, 120, 110, 110, 60, 90, 90, 200 },
    { 90, 90, 90, 110, 110, 90, 90, 90, 200 },
    { 70, 90, 90, 110, 110, 90, 90, 70, 200 },
};

const int Servo_Prg_6_Step = 8;
const int Servo_Prg_6[][ALLMATRIX] PROGMEM = {
    { 70, 90, 90, 110, 110, 90, 90, 70, 200 },
    { 90, 90, 90, 110, 110, 90, 90, 90, 200 },
    { 90, 135, 90, 110, 110, 90, 135, 90, 200 },
    { 70, 135, 90, 110, 110, 90, 135, 70, 200 },
    { 70, 135, 90, 90, 90, 90, 135, 70, 200 },
    { 70, 135, 135, 90, 90, 135, 135, 70, 200 },
    { 70, 135, 135, 110, 110, 135, 135, 70, 200 },
    { 70, 90, 90, 110, 110, 90, 90, 70, 200 },
};

const int Servo_Prg_7_Step = 8;
const int Servo_Prg_7[][ALLMATRIX] PROGMEM = {
    { 70, 90, 90, 110, 110, 90, 90, 70, 200 },
    { 70, 90, 90, 90, 90, 90, 90, 70, 200 },
    { 70, 90, 45, 90, 90, 45, 90, 70, 200 },
    { 70, 90, 45, 110, 110, 45, 90, 70, 200 },
    { 90, 90, 45, 110, 110, 45, 90, 90, 200 },
    { 90, 45, 45, 110, 110, 45, 45, 90, 200 },
    { 70, 45, 45, 110, 110, 45, 45, 70, 200 },
    { 70, 90, 90, 110, 110, 90, 90, 70, 200 },
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

// ============ Speed Funktionen ============

void setSpeed(int speed) {
    if (speed < 10) speed = 10;
    if (speed > 200) speed = 200;
    speedMultiplier = speed;
}

int getSpeed() {
    return speedMultiplier;
}

// ============ Terrain Funktionen ============

void setTerrainMode(TerrainMode mode) {
    currentTerrainMode = mode;
    
    // Setze Blend-Target basierend auf Modus
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
    
    Serial.printf("[Terrain] Modus: %s (Target: %d)\n", getTerrainModeName(), terrainBlendTarget);
}

TerrainMode getTerrainMode() {
    return currentTerrainMode;
}

const char* getTerrainModeName() {
    switch (currentTerrainMode) {
        case TERRAIN_UPHILL: return "Uphill";
        case TERRAIN_DOWNHILL: return "Downhill";
        default: return "Normal";
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

// Berechnet Terrain-Offset für eine Pfote
// Layout (von oben): UR(0), UL(4) = VORNE, LR(3), LL(7) = HINTEN
//
// Uphill (terrainBlendCurrent > 0): Körper vorne TIEFER
//   -> Vordere Beine müssen RUNTER (Körper senkt sich vorne)
//   -> Hintere Beine müssen HOCH (Körper hebt sich hinten)
//
// "Runter" bedeutet: Bein länger/gestreckter
// "Hoch" bedeutet: Bein kürzer/angehoben
//
// Aber: Rechts und Links haben UMGEKEHRTE Servo-Richtungen!
// Rechts: kleinerer Winkel = Bein hebt an
// Links: größerer Winkel = Bein hebt an
// -> Das wird über liftSign[] korrigiert
int getTerrainAdjustment(int iServo) {
    // Nur Pfoten anpassen (0=UR, 3=LR, 4=UL, 7=LL)
    bool isPaw = (iServo == 0 || iServo == 3 || iServo == 4 || iServo == 7);
    if (!isPaw || terrainBlendCurrent == 0) {
        return 0;
    }
    
    bool isFront = (iServo == 0 || iServo == 4);  // UR, UL sind vorne
    
    // Berechne gewünschte Höhenänderung (positiv = Bein soll HOCH)
    // Bei Uphill (blend > 0): Vorne RUNTER (-), Hinten HOCH (+)
    int heightOffset;
    if (isFront) {
        heightOffset = -terrainBlendCurrent;  // Vorne: bei Uphill runter (negativ)
    } else {
        heightOffset = terrainBlendCurrent;   // Hinten: bei Uphill hoch (positiv)
    }
    
    // Konvertiere Höhen-Offset zu Servo-Winkel-Delta über liftSign
    // liftSign[i] gibt an, welches Vorzeichen "Bein hoch" bedeutet
    return heightOffset * liftSign[iServo];
}

// ============ Servo Offset Funktionen ============

void setServoOffset(int iServo, int offset) {
    if (iServo >= 0 && iServo < ALLSERVOS) {
        Servo_Offset[iServo] = offset;
    }
}

int getServoOffset(int iServo) {
    if (iServo >= 0 && iServo < ALLSERVOS) {
        return Servo_Offset[iServo];
    }
    return 0;
}

void setAllServoOffsets(int offsets[]) {
    for (int i = 0; i < ALLSERVOS; i++) {
        Servo_Offset[i] = offsets[i];
    }
}

void resetServoOffsets() {
    for (int i = 0; i < ALLSERVOS; i++) {
        Servo_Offset[i] = 0;
    }
}

bool saveOffsetsToFile() {
    File f = LittleFS.open("/offsets.dat", "w");
    if (!f) {
        Serial.println("[Offsets] Fehler beim Öffnen zum Schreiben");
        return false;
    }
    f.write((uint8_t*)Servo_Offset, sizeof(Servo_Offset));
    f.close();
    Serial.println("[Offsets] Gespeichert");
    return true;
}

bool loadOffsetsFromFile() {
    if (!LittleFS.exists("/offsets.dat")) {
        Serial.println("[Offsets] Keine Datei vorhanden");
        return false;
    }
    File f = LittleFS.open("/offsets.dat", "r");
    if (!f) {
        Serial.println("[Offsets] Fehler beim Öffnen zum Lesen");
        return false;
    }
    if (f.size() != sizeof(Servo_Offset)) {
        Serial.println("[Offsets] Ungültige Dateigröße");
        f.close();
        return false;
    }
    f.read((uint8_t*)Servo_Offset, sizeof(Servo_Offset));
    f.close();
    Serial.println("[Offsets] Geladen");
    return true;
}

// ============ Servo Control ============

void Set_PWM_to_Servo(int iServo, int iValue) {
    // Terrain-Anpassung holen (bereits mit liftSign korrigiert!)
    int terrainAdj = getTerrainAdjustment(iServo);
    
    // Kalibrierter Wert mit Offset und Terrain
    // KEINE servoDir-Inversion hier - die Bewegungsmatrizen sind bereits kalibriert!
    int calibratedValue = iValue + Servo_Offset[iServo] + terrainAdj;
    
    // Begrenzen
    calibratedValue = constrain(calibratedValue, PWMRES_Min, PWMRES_Max);
    
    // In PWM-Wert umrechnen
    int NewPWM = map(calibratedValue, PWMRES_Min, PWMRES_Max, SERVOMIN, SERVOMAX);

    switch (iServo) {
        case 0: servo_14.writeMicroseconds(NewPWM); break;
        case 1: servo_12.writeMicroseconds(NewPWM); break;
        case 2: servo_13.writeMicroseconds(NewPWM); break;
        case 3: servo_15.writeMicroseconds(NewPWM); break;
        case 4: servo_16.writeMicroseconds(NewPWM); break;
        case 5: servo_5.writeMicroseconds(NewPWM); break;
        case 6: servo_4.writeMicroseconds(NewPWM); break;
        case 7: servo_2.writeMicroseconds(NewPWM); break;
    }
}

// ============ Blockierende Motion Engine ============

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

        // Exakte Endposition
        for (int ServoIndex = 0; ServoIndex < ALLSERVOS; ServoIndex++) {
            int target = pgm_read_word(&iMatrix[MainLoopIndex][ServoIndex]);
            Set_PWM_to_Servo(ServoIndex, target);
        }

        // Update Running_Servo_POS
        for (int Index = 0; Index < ALLMATRIX; Index++) {
            Running_Servo_POS[Index] = pgm_read_word(&iMatrix[MainLoopIndex][Index]);
        }
    }
}

// ============ Nicht-blockierende Motion Engine ============

void motion_start(const int matrix[][9], int steps) {
    if (steps <= 0 || matrix == nullptr) return;
    
    motionState.matrix = matrix;
    motionState.totalSteps = steps;
    motionState.currentStep = 0;
    motionState.sequenceComplete = false;
    
    // Aktuelle Position als Startpunkt
    for (int i = 0; i < ALLSERVOS; i++) {
        motionState.fromPose[i] = Running_Servo_POS[i];
        motionState.toPose[i] = pgm_read_word(&matrix[0][i]);
    }
    
    // Timing berechnen
    int originalTime = pgm_read_word(&matrix[0][ALLMATRIX - 1]);
    int timePercent = (110 - speedMultiplier) / 3;
    if (timePercent < 5) timePercent = 5;
    motionState.segmentDuration = (originalTime * timePercent) / 100;
    if (motionState.segmentDuration < 20) motionState.segmentDuration = 20;
    
    motionState.segmentStartMs = millis();
    motionState.active = true;
    
    Serial.printf("[Motion] Start: %d steps, %dms/step\n", steps, motionState.segmentDuration);
}

bool motion_tick(unsigned long nowMs) {
    if (!motionState.active) return false;
    
    // Terrain sanft anpassen
    terrain_blend_tick();
    
    unsigned long elapsed = nowMs - motionState.segmentStartMs;
    
    // Interpolation berechnen (0.0 - 1.0)
    float alpha = (float)elapsed / (float)motionState.segmentDuration;
    if (alpha > 1.0f) alpha = 1.0f;
    
    // Smoothstep Easing für flüssigere Bewegung
    float eased = alpha * alpha * (3.0f - 2.0f * alpha);
    
    // Alle Servos interpolieren
    for (int i = 0; i < ALLSERVOS; i++) {
        int start = motionState.fromPose[i];
        int target = motionState.toPose[i];
        int value = start + (int)((target - start) * eased);
        Set_PWM_to_Servo(i, value);
    }
    
    // Segment abgeschlossen?
    if (elapsed >= (unsigned long)motionState.segmentDuration) {
        // Exakte Endposition setzen
        for (int i = 0; i < ALLSERVOS; i++) {
            Set_PWM_to_Servo(i, motionState.toPose[i]);
            Running_Servo_POS[i] = motionState.toPose[i];
        }
        
        // Nächster Step?
        motionState.currentStep++;
        
        if (motionState.currentStep >= motionState.totalSteps) {
            // Sequenz beendet
            motionState.active = false;
            motionState.sequenceComplete = true;
            Serial.println("[Motion] Sequenz beendet");
            return false;
        }
        
        // Nächstes Segment vorbereiten
        for (int i = 0; i < ALLSERVOS; i++) {
            motionState.fromPose[i] = Running_Servo_POS[i];
            motionState.toPose[i] = pgm_read_word(&motionState.matrix[motionState.currentStep][i]);
        }
        
        int originalTime = pgm_read_word(&motionState.matrix[motionState.currentStep][ALLMATRIX - 1]);
        int timePercent = (110 - speedMultiplier) / 3;
        if (timePercent < 5) timePercent = 5;
        motionState.segmentDuration = (originalTime * timePercent) / 100;
        if (motionState.segmentDuration < 20) motionState.segmentDuration = 20;
        
        motionState.segmentStartMs = nowMs;
    }
    
    return true;
}

void motion_stop() {
    motionState.active = false;
    motionState.sequenceComplete = true;
}

bool motion_is_active() {
    return motionState.active;
}

bool motion_sequence_complete() {
    return motionState.sequenceComplete;
}

void motion_reset_sequence_flag() {
    motionState.sequenceComplete = false;
}

// ============ Initialisierung ============

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

// ============ Bewegungsfunktionen (blockierend) ============

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

void forward() {
    Servo_PROGRAM_Run(Servo_Prg_2, Servo_Prg_2_Step);
}

void back() {
    Servo_PROGRAM_Run(Servo_Prg_3, Servo_Prg_3_Step);
}

void turnleft() {
    Servo_PROGRAM_Run(Servo_Prg_6, Servo_Prg_6_Step);
}

void turnright() {
    Servo_PROGRAM_Run(Servo_Prg_7, Servo_Prg_7_Step);
}

void leftmove() {
    Servo_PROGRAM_Run(Servo_Prg_4, Servo_Prg_4_Step);
}

void rightmove() {
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
