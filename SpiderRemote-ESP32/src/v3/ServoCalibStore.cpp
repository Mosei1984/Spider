// =============================================================================
// ServoCalibStore.cpp - Servo-Kalibrierung NVS Speicherung
// =============================================================================
#include "ServoCalibStore.h"
#include <Preferences.h>

static Preferences prefs;
static const char* NVS_NAMESPACE = "servocalib";

void ServoCalibStore::begin() {
    prefs.begin(NVS_NAMESPACE, true);
    valid_ = prefs.getBool("valid", false);
    prefs.end();
    Serial.printf("[ServoCalib] Valid: %s\n", valid_ ? "yes" : "no");
}

void ServoCalibStore::load(ServoCalibParams* servos, uint8_t count) {
    if (count > 8) count = 8;
    
    prefs.begin(NVS_NAMESPACE, true);
    valid_ = prefs.getBool("valid", false);
    
    if (valid_) {
        for (uint8_t i = 0; i < count; i++) {
            char key[16];
            snprintf(key, sizeof(key), "s%d_off", i);
            servos[i].offset = prefs.getInt(key, 0);
            
            snprintf(key, sizeof(key), "s%d_min", i);
            servos[i].minAngle = prefs.getInt(key, 20);
            
            snprintf(key, sizeof(key), "s%d_max", i);
            servos[i].maxAngle = prefs.getInt(key, 160);
            
            snprintf(key, sizeof(key), "s%d_ctr", i);
            servos[i].centerAngle = prefs.getInt(key, 90);
        }
        Serial.println(F("[ServoCalib] Loaded from NVS"));
    }
    prefs.end();
}

void ServoCalibStore::save(const ServoCalibParams* servos, uint8_t count) {
    if (count > 8) count = 8;
    
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putBool("valid", true);
    
    for (uint8_t i = 0; i < count; i++) {
        char key[16];
        snprintf(key, sizeof(key), "s%d_off", i);
        prefs.putInt(key, servos[i].offset);
        
        snprintf(key, sizeof(key), "s%d_min", i);
        prefs.putInt(key, servos[i].minAngle);
        
        snprintf(key, sizeof(key), "s%d_max", i);
        prefs.putInt(key, servos[i].maxAngle);
        
        snprintf(key, sizeof(key), "s%d_ctr", i);
        prefs.putInt(key, servos[i].centerAngle);
    }
    
    prefs.end();
    valid_ = true;
    Serial.println(F("[ServoCalib] Saved to NVS"));
}
