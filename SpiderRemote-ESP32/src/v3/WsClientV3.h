// =============================================================================
// WsClientV3.h - Erweiterter WebSocket Client für v3 Protokoll
// =============================================================================
// SpiderRemote-ESP32 v3 Extension
// Erweitert WsClient um:
//   - setWalkParams Command
//   - moveStartEx mit optionalen Parametern
//   - Servo-Kalibrierungs-Commands
// =============================================================================
#pragma once
#include <Arduino.h>
#include "../WsClient.h"
#include "WalkParams.h"

class WsClientV3 : public WsClient {
public:
    // ==========================================================================
    // Erweiterte Walk-Parameter Commands
    // ==========================================================================
    
    // Setzt globale Walk-Parameter (persistent auf Spider bis Änderung)
    void sendSetWalkParams(const WalkParams& params);
    
    // Erweiterte moveStart mit optionalen Parameter-Overrides
    void sendMoveStartEx(const char* name, const WalkParams* override = nullptr);
    
    // Einzelne Parameter ändern
    void sendSetStride(float stride);
    void sendSetSubSteps(uint8_t steps);
    void sendSetTimingProfile(TimingProfile profile);
    void sendSetSwingMul(float mult);
    void sendSetStanceMul(float mult);
    void sendEnableRamp(bool enable, uint8_t cycles = 3);
    
    // ==========================================================================
    // Servo-Kalibrierungs-Commands
    // ==========================================================================
    
    // Offset für einen Servo setzen
    void sendSetServoOffset(uint8_t servo, int offset);
    
    // Limits für einen Servo setzen
    void sendSetServoLimits(uint8_t servo, int minAngle, int maxAngle, int centerAngle);
    
    // Alle Kalibrierungsdaten eines Servos setzen
    void sendSetServoCalib(uint8_t servo, const ServoCalibParams& params);
    
    // Kalibrierung speichern/laden anfordern
    void sendSaveCalib();
    void sendLoadCalib();
    
    // Kalibrierung vom Bot anfordern (Request)
    void sendGetServoCalib();
    
    // Kalibrierungs-Lock setzen
    void sendSetCalibLock(bool locked);
    
    // Callback für empfangene Servo-Kalibrierung
    typedef void (*ServoCalibCallback)(uint8_t servo, const ServoCalibParams& params);
    void setServoCalibCallback(ServoCalibCallback cb) { servoCalibCallback = cb; }
    
    // Muss im Loop aufgerufen werden um Responses zu verarbeiten
    void processResponse(const char* json);
    
    // ==========================================================================
    // Aktuelle Parameter (lokal gecached)
    // ==========================================================================
    
    const WalkParams& getWalkParams() const { return currentParams; }
    void setLocalWalkParams(const WalkParams& params) { currentParams = params; }
    
private:
    WalkParams currentParams;
    ServoCalibCallback servoCalibCallback = nullptr;
};
