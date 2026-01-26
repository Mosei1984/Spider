// =============================================================================
// ServoCalibStore.h - Servo-Kalibrierung NVS Speicherung
// =============================================================================
#pragma once
#include <Arduino.h>
#include "WalkParams.h"

class ServoCalibStore {
public:
    void begin();
    
    // Laden/Speichern
    void load(ServoCalibParams* servos, uint8_t count);
    void save(const ServoCalibParams* servos, uint8_t count);
    
    // Status
    bool isValid() const { return valid_; }
    
private:
    bool valid_ = false;
};
