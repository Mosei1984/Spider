// =============================================================================
// ServoCalibration.cpp - Implementierung der Servo-Kalibrierung
// =============================================================================
#include "ServoCalibration.h"
#include "../gait/GaitRuntime.h"
#include <LittleFS.h>

namespace ServoCalibration {

// =============================================================================
// Statische Daten
// =============================================================================
static CalibrationData calibData;
static const char* CALIB_FILE = "/servo_calib_v3.dat";
static const uint32_t MAGIC = 0x53564F33;  // "SVO3"

// =============================================================================
// Initialisierung
// =============================================================================
void init() {
    calibData = CalibrationData();
    
    if (!load()) {
        Serial.println(F("[ServoCalib] Keine Kalibrierung, verwende Defaults"));
        // Standard-Werte setzen basierend auf bekannter Mechanik
        // Diese können später per Remote angepasst werden
        for (uint8_t i = 0; i < SERVO_COUNT; i++) {
            calibData.offset[i] = 0;
            calibData.limits[i] = ServoLimits(20, 160, 90);
        }
        calibData.valid = true;
    }
    
    // Mit GaitConfig synchronisieren
    syncToGaitConfig();
    
    printAll();
}

// =============================================================================
// Offset-Verwaltung
// =============================================================================
void setOffset(uint8_t servo, int value) {
    if (servo >= SERVO_COUNT) return;
    
    // Clamp auf sinnvollen Bereich
    if (value < -30) value = -30;
    if (value > 30) value = 30;
    
    calibData.offset[servo] = value;
    Serial.printf("[ServoCalib] Offset[%d] = %d\n", servo, value);
}

int getOffset(uint8_t servo) {
    if (servo >= SERVO_COUNT) return 0;
    return calibData.offset[servo];
}

void resetOffsets() {
    for (uint8_t i = 0; i < SERVO_COUNT; i++) {
        calibData.offset[i] = 0;
    }
    Serial.println(F("[ServoCalib] Offsets zurückgesetzt"));
}

// =============================================================================
// Limits-Verwaltung
// =============================================================================
void setLimits(uint8_t servo, int minAngle, int maxAngle, int centerAngle) {
    if (servo >= SERVO_COUNT) return;
    
    // Validierung
    if (minAngle < 0) minAngle = 0;
    if (maxAngle > 180) maxAngle = 180;
    if (minAngle >= maxAngle) {
        Serial.println(F("[ServoCalib] Ungültige Limits ignoriert"));
        return;
    }
    if (centerAngle < minAngle) centerAngle = minAngle;
    if (centerAngle > maxAngle) centerAngle = maxAngle;
    
    calibData.limits[servo].minAngle = minAngle;
    calibData.limits[servo].maxAngle = maxAngle;
    calibData.limits[servo].centerAngle = centerAngle;
    
    // Sofort mit GaitConfig synchronisieren
    GaitRuntime::setServoLimits(servo, minAngle, maxAngle, centerAngle);
    
    Serial.printf("[ServoCalib] Limits[%d]: min=%d, max=%d, center=%d\n", 
        servo, minAngle, maxAngle, centerAngle);
}

int getMinAngle(uint8_t servo) {
    if (servo >= SERVO_COUNT) return 0;
    return calibData.limits[servo].minAngle;
}

int getMaxAngle(uint8_t servo) {
    if (servo >= SERVO_COUNT) return 180;
    return calibData.limits[servo].maxAngle;
}

int getCenterAngle(uint8_t servo) {
    if (servo >= SERVO_COUNT) return 90;
    return calibData.limits[servo].centerAngle;
}

void resetLimits() {
    for (uint8_t i = 0; i < SERVO_COUNT; i++) {
        calibData.limits[i] = ServoLimits(20, 160, 90);
    }
    syncToGaitConfig();
    Serial.println(F("[ServoCalib] Limits zurückgesetzt"));
}

// =============================================================================
// Winkel-Transformation
// =============================================================================
int applyOffset(uint8_t servo, int rawAngle) {
    if (servo >= SERVO_COUNT) return rawAngle;
    return rawAngle + calibData.offset[servo];
}

int clampToLimits(uint8_t servo, int angle) {
    if (servo >= SERVO_COUNT) return angle;
    
    const ServoLimits& lim = calibData.limits[servo];
    if (angle < lim.minAngle) return lim.minAngle;
    if (angle > lim.maxAngle) return lim.maxAngle;
    return angle;
}

// =============================================================================
// Persistenz
// =============================================================================
bool save() {
    File f = LittleFS.open(CALIB_FILE, "w");
    if (!f) {
        Serial.println(F("[ServoCalib] Speichern fehlgeschlagen"));
        return false;
    }
    
    // Magic schreiben
    f.write((uint8_t*)&MAGIC, sizeof(MAGIC));
    
    // Offsets
    f.write((uint8_t*)calibData.offset, sizeof(calibData.offset));
    
    // Limits
    for (uint8_t i = 0; i < SERVO_COUNT; i++) {
        f.write((uint8_t*)&calibData.limits[i], sizeof(ServoLimits));
    }
    
    f.close();
    Serial.println(F("[ServoCalib] Gespeichert"));
    return true;
}

bool load() {
    File f = LittleFS.open(CALIB_FILE, "r");
    if (!f) {
        return false;
    }
    
    // Magic prüfen
    uint32_t magic = 0;
    f.read((uint8_t*)&magic, sizeof(magic));
    if (magic != MAGIC) {
        Serial.println(F("[ServoCalib] Datei-Version ungültig"));
        f.close();
        return false;
    }
    
    // Offsets laden
    f.read((uint8_t*)calibData.offset, sizeof(calibData.offset));
    
    // Limits laden
    for (uint8_t i = 0; i < SERVO_COUNT; i++) {
        f.read((uint8_t*)&calibData.limits[i], sizeof(ServoLimits));
    }
    
    f.close();
    calibData.valid = true;
    Serial.println(F("[ServoCalib] Geladen"));
    return true;
}

// =============================================================================
// Synchronisierung mit GaitConfig
// =============================================================================
void syncToGaitConfig() {
    for (uint8_t i = 0; i < SERVO_COUNT; i++) {
        GaitRuntime::setServoLimits(i, 
            calibData.limits[i].minAngle,
            calibData.limits[i].maxAngle,
            calibData.limits[i].centerAngle);
    }
    Serial.println(F("[ServoCalib] Mit GaitConfig synchronisiert"));
}

// =============================================================================
// Debug
// =============================================================================
void printAll() {
    Serial.println(F("=== Servo-Kalibrierung ==="));
    for (uint8_t i = 0; i < SERVO_COUNT; i++) {
        Serial.printf("  [%d] offset=%+3d, min=%3d, max=%3d, center=%3d\n",
            i, calibData.offset[i],
            calibData.limits[i].minAngle,
            calibData.limits[i].maxAngle,
            calibData.limits[i].centerAngle);
    }
    Serial.println(F("=========================="));
}

const CalibrationData& getData() {
    return calibData;
}

} // namespace ServoCalibration
