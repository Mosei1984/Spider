// =============================================================================
// ServoCalibration.h - Servo-Kalibrierung mit Limits pro Servo
// =============================================================================
// v3 Calibration Module für ESP8266 Spider Controller
// Features:
//   - Offset pro Servo
//   - Min/Max/Center Limits pro Servo
//   - Persistenz in LittleFS
//   - Live-Anpassung über WebSocket
// =============================================================================
#ifndef SERVO_CALIBRATION_H
#define SERVO_CALIBRATION_H

#include <Arduino.h>
#include "../gait/GaitConfig.h"

namespace ServoCalibration {

// =============================================================================
// Kalibrierungs-Datenstruktur (erweitert)
// =============================================================================
struct CalibrationData {
    int offset[SERVO_COUNT];           // Offset-Korrektur pro Servo
    ServoLimits limits[SERVO_COUNT];   // Min/Max/Center pro Servo
    bool valid;                        // Magic-Check für Dateivalidierung
    
    CalibrationData() {
        valid = false;
        for (uint8_t i = 0; i < SERVO_COUNT; i++) {
            offset[i] = 0;
            limits[i] = ServoLimits(20, 160, 90);  // Konservative Defaults
        }
    }
};

// =============================================================================
// API
// =============================================================================

// Initialisierung (lädt gespeicherte Daten)
void init();

// Offset-Verwaltung
void setOffset(uint8_t servo, int value);
int getOffset(uint8_t servo);
void resetOffsets();

// Limits-Verwaltung
void setLimits(uint8_t servo, int minAngle, int maxAngle, int centerAngle);
int getMinAngle(uint8_t servo);
int getMaxAngle(uint8_t servo);
int getCenterAngle(uint8_t servo);
void resetLimits();

// Offset auf Winkel anwenden (wird von Set_PWM_to_Servo genutzt)
int applyOffset(uint8_t servo, int rawAngle);

// Winkel auf Limits clampen
int clampToLimits(uint8_t servo, int angle);

// Persistenz
bool save();
bool load();

// Debug: Alle Werte ausgeben
void printAll();

// Gesamte Kalibrierungsdaten holen (für WebSocket-Antwort)
const CalibrationData& getData();

// Kalibrierung mit GaitConfig synchronisieren
void syncToGaitConfig();

} // namespace ServoCalibration

#endif // SERVO_CALIBRATION_H
