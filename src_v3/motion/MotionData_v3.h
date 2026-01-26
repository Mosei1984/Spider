// =============================================================================
// MotionData_v3.h - Erweiterte Motion-Daten für v3 Gait Runtime
// =============================================================================
// Basiert auf original MotionData.h, erweitert um GaitRuntime Integration
// =============================================================================
#ifndef MOTION_DATA_V3_H
#define MOTION_DATA_V3_H

#include <Servo.h>
#include "../gait/GaitConfig.h"

// =============================================================================
// Servo-Objekte (extern, definiert in MotionData_v3.cpp)
// =============================================================================
extern Servo servo_14;
extern Servo servo_12;
extern Servo servo_13;
extern Servo servo_15;
extern Servo servo_16;
extern Servo servo_5;
extern Servo servo_4;
extern Servo servo_2;

// =============================================================================
// Konstanten
// =============================================================================
extern const int PWMRES_Min;
extern const int PWMRES_Max;
extern const int ALLMATRIX;
extern const int ALLSERVOS;
extern const int SERVOMIN;
extern const int SERVOMAX;

// =============================================================================
// Globale Variablen
// =============================================================================
extern int Running_Servo_POS[];
extern int BASEDELAYTIME;
extern int speedMultiplier;
extern int Servo_Offset[];

// Lift-Sign für Terrain-Offsets
extern const int8_t liftSign[];

// =============================================================================
// Terrain-Modus
// =============================================================================
enum TerrainMode {
    TERRAIN_NORMAL = 0,
    TERRAIN_UPHILL = 1,
    TERRAIN_DOWNHILL = 2
};

extern TerrainMode currentTerrainMode;
extern int terrainBlendCurrent;
extern int terrainBlendTarget;

// =============================================================================
// Terrain-Funktionen
// =============================================================================
void setTerrainMode(TerrainMode mode);
TerrainMode getTerrainMode();
const char* getTerrainModeName();
void terrain_blend_tick();
int getTerrainAdjustment(int iServo);

// =============================================================================
// Speed-Funktionen
// =============================================================================
void setSpeed(int speed);
int getSpeed();

// =============================================================================
// Servo-Control (low-level)
// =============================================================================
void Set_PWM_to_Servo(int iServo, int iValue);

// =============================================================================
// Legacy Blocking Motion Engine (für Dance/Hello etc.)
// =============================================================================
void Servo_PROGRAM_Run(const int iMatrix[][9], int iSteps);
void Servo_PROGRAM_Zero();

// =============================================================================
// Blockierende Bewegungsfunktionen (Legacy, für nicht-kontinuierliche Aktionen)
// =============================================================================
void standby();
void sleep();
void lie();
void forward_blocking();
void back_blocking();
void turnleft_blocking();
void turnright_blocking();
void leftmove_blocking();
void rightmove_blocking();
void hello();
void dance1();
void dance2();
void dance3();
void pushup();
void fighting();
void calibpose();

// =============================================================================
// Bewegungsmatrizen (PROGMEM)
// =============================================================================
extern const int Servo_Prg_1[][9];
extern const int Servo_Prg_1_Step;
extern const int Servo_Prg_2[][9];
extern const int Servo_Prg_2_Step;
extern const int Servo_Prg_3[][9];
extern const int Servo_Prg_3_Step;
extern const int Servo_Prg_4[][9];
extern const int Servo_Prg_4_Step;
extern const int Servo_Prg_5[][9];
extern const int Servo_Prg_5_Step;
extern const int Servo_Prg_6[][9];
extern const int Servo_Prg_6_Step;
extern const int Servo_Prg_7[][9];
extern const int Servo_Prg_7_Step;
extern const int Servo_Prg_8[][9];
extern const int Servo_Prg_8_Step;
extern const int Servo_Prg_9[][9];
extern const int Servo_Prg_9_Step;
extern const int Servo_Prg_10[][9];
extern const int Servo_Prg_10_Step;
extern const int Servo_Prg_11[][9];
extern const int Servo_Prg_11_Step;
extern const int Servo_Prg_12[][9];
extern const int Servo_Prg_12_Step;
extern const int Servo_Prg_13[][9];
extern const int Servo_Prg_13_Step;
extern const int Servo_Prg_14[][9];
extern const int Servo_Prg_14_Step;
extern const int Servo_Prg_15[][9];
extern const int Servo_Prg_15_Step;

#endif // MOTION_DATA_V3_H
