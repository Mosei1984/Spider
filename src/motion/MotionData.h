#ifndef MOTION_DATA_H
#define MOTION_DATA_H

#include <Servo.h>

extern Servo servo_14;
extern Servo servo_12;
extern Servo servo_13;
extern Servo servo_15;
extern Servo servo_16;
extern Servo servo_5;
extern Servo servo_4;
extern Servo servo_2;

extern const int PWMRES_Min;
extern const int PWMRES_Max;
extern const int ALLMATRIX;
extern const int ALLSERVOS;
extern const int SERVOMIN;
extern const int SERVOMAX;

extern int Running_Servo_POS[];
extern int BASEDELAYTIME;
extern int speedMultiplier;
extern int Servo_Offset[];

// Lift-Sign für Terrain-Offsets (Delta-Vorzeichen pro Pfote)
extern const int8_t liftSign[];

// Terrain-Modus für Steigungen
enum TerrainMode {
    TERRAIN_NORMAL = 0,
    TERRAIN_UPHILL = 1,
    TERRAIN_DOWNHILL = 2
};

extern TerrainMode currentTerrainMode;
extern int terrainBlendCurrent;  // Aktueller Blend-Wert
extern int terrainBlendTarget;   // Ziel-Wert

// Nicht-blockierende Motion-Engine
struct MotionState {
    volatile bool active;
    unsigned long segmentStartMs;
    int segmentDuration;
    int currentStep;
    int totalSteps;
    int fromPose[8];
    int toPose[8];
    const int (*matrix)[9];  // Pointer auf Bewegungsmatrix
    bool sequenceComplete;   // true wenn Sequenz fertig
};

extern MotionState motionState;

// Motion Engine Funktionen
void motion_start(const int matrix[][9], int steps);
bool motion_tick(unsigned long nowMs);  // true = Motion läuft noch
void motion_stop();
bool motion_is_active();
bool motion_sequence_complete();
void motion_reset_sequence_flag();

// Terrain Funktionen
void setTerrainMode(TerrainMode mode);
TerrainMode getTerrainMode();
const char* getTerrainModeName();
void terrain_blend_tick();  // Sanfte Terrain-Übergänge
int getTerrainAdjustment(int iServo);

// Speed Funktionen
void setSpeed(int speed);
int getSpeed();

// Servo Offset Funktionen
void setServoOffset(int iServo, int offset);
int getServoOffset(int iServo);
void resetServoOffsets();
bool saveOffsetsToFile();
bool loadOffsetsFromFile();

// Servo Control
void Set_PWM_to_Servo(int iServo, int iValue);
void Servo_PROGRAM_Run(const int iMatrix[][9], int iSteps);
void Servo_PROGRAM_Zero();

// Bewegungsfunktionen (blockierend)
void standby();
void sleep();
void lie();
void forward();
void back();
void turnleft();
void turnright();
void leftmove();
void rightmove();
void hello();
void dance1();
void dance2();
void dance3();
void pushup();
void fighting();

// Bewegungsmatrizen extern deklarieren
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

#endif
