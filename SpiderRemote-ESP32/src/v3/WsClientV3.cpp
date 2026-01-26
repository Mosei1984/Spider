// =============================================================================
// WsClientV3.cpp - Implementierung des erweiterten WebSocket Clients
// =============================================================================
#include "WsClientV3.h"
#include <Arduino.h>

// =============================================================================
// Walk-Parameter Commands
// =============================================================================

void WsClientV3::sendSetWalkParams(const WalkParams& params) {
    currentParams = params;
    currentParams.validate();
    
    char buf[256];
    snprintf(buf, sizeof(buf),
        "{\"type\":\"setWalkParams\","
        "\"stride\":%.2f,"
        "\"subSteps\":%d,"
        "\"profile\":%d,"
        "\"swingMul\":%.2f,"
        "\"stanceMul\":%.2f,"
        "\"rampEnabled\":%s,"
        "\"rampCycles\":%d}",
        currentParams.stride,
        currentParams.subSteps,
        (int)currentParams.profile,
        currentParams.swingMul,
        currentParams.stanceMul,
        currentParams.rampEnabled ? "true" : "false",
        currentParams.rampCycles
    );
    
    sendImmediate(buf);
    Serial.printf("[WsV3] WalkParams: stride=%.2f, subSteps=%d\n", 
        currentParams.stride, currentParams.subSteps);
}

void WsClientV3::sendMoveStartEx(const char* name, const WalkParams* override) {
    if (!name) return;
    
    if (override) {
        // Mit Parameter-Override
        char buf[256];
        snprintf(buf, sizeof(buf),
            "{\"type\":\"moveStart\","
            "\"name\":\"%s\","
            "\"stride\":%.2f,"
            "\"subSteps\":%d,"
            "\"profile\":%d}",
            name,
            override->stride,
            override->subSteps,
            (int)override->profile
        );
        sendImmediate(buf);
    } else {
        // Standard moveStart (nutzt vorher gesetzte WalkParams)
        sendMoveStart(name);
    }
}

void WsClientV3::sendSetStride(float stride) {
    currentParams.stride = stride;
    currentParams.validate();
    
    char buf[64];
    snprintf(buf, sizeof(buf), "{\"type\":\"setStride\",\"value\":%.2f}", currentParams.stride);
    sendImmediate(buf);
}

void WsClientV3::sendSetSubSteps(uint8_t steps) {
    currentParams.subSteps = steps;
    currentParams.validate();
    
    char buf[64];
    snprintf(buf, sizeof(buf), "{\"type\":\"setSubSteps\",\"value\":%d}", currentParams.subSteps);
    sendImmediate(buf);
}

void WsClientV3::sendSetTimingProfile(TimingProfile profile) {
    currentParams.profile = profile;
    
    char buf[64];
    snprintf(buf, sizeof(buf), "{\"type\":\"setTimingProfile\",\"value\":%d}", (int)profile);
    sendImmediate(buf);
}

void WsClientV3::sendSetSwingMul(float mult) {
    currentParams.swingMul = mult;
    currentParams.validate();
    
    char buf[64];
    snprintf(buf, sizeof(buf), "{\"type\":\"setSwingMul\",\"value\":%.2f}", currentParams.swingMul);
    sendImmediate(buf);
}

void WsClientV3::sendSetStanceMul(float mult) {
    currentParams.stanceMul = mult;
    currentParams.validate();
    
    char buf[64];
    snprintf(buf, sizeof(buf), "{\"type\":\"setStanceMul\",\"value\":%.2f}", currentParams.stanceMul);
    sendImmediate(buf);
}

void WsClientV3::sendEnableRamp(bool enable, uint8_t cycles) {
    currentParams.rampEnabled = enable;
    currentParams.rampCycles = cycles;
    currentParams.validate();
    
    char buf[96];
    snprintf(buf, sizeof(buf), 
        "{\"type\":\"setRamp\",\"enabled\":%s,\"cycles\":%d}", 
        enable ? "true" : "false", 
        currentParams.rampCycles
    );
    sendImmediate(buf);
}

// =============================================================================
// Servo-Kalibrierungs-Commands
// =============================================================================

void WsClientV3::sendSetServoOffset(uint8_t servo, int offset) {
    if (servo >= 8) return;
    
    // Clamp
    if (offset < -30) offset = -30;
    if (offset > 30) offset = 30;
    
    char buf[64];
    snprintf(buf, sizeof(buf), 
        "{\"type\":\"setServoOffset\",\"servo\":%d,\"offset\":%d}", 
        servo, offset
    );
    sendImmediate(buf);
}

void WsClientV3::sendSetServoLimits(uint8_t servo, int minAngle, int maxAngle, int centerAngle) {
    if (servo >= 8) return;
    
    // Basic validation
    if (minAngle < 0) minAngle = 0;
    if (maxAngle > 180) maxAngle = 180;
    if (centerAngle < minAngle) centerAngle = minAngle;
    if (centerAngle > maxAngle) centerAngle = maxAngle;
    
    char buf[128];
    snprintf(buf, sizeof(buf), 
        "{\"type\":\"setServoLimits\",\"servo\":%d,\"min\":%d,\"max\":%d,\"center\":%d}", 
        servo, minAngle, maxAngle, centerAngle
    );
    sendImmediate(buf);
}

void WsClientV3::sendSetServoCalib(uint8_t servo, const ServoCalibParams& params) {
    if (servo >= 8) return;
    
    ServoCalibParams validated = params;
    validated.validate();
    
    char buf[160];
    snprintf(buf, sizeof(buf), 
        "{\"type\":\"setServoCalib\",\"servo\":%d,"
        "\"offset\":%d,\"min\":%d,\"max\":%d,\"center\":%d}", 
        servo, 
        validated.offset, 
        validated.minAngle, 
        validated.maxAngle, 
        validated.centerAngle
    );
    sendImmediate(buf);
}

void WsClientV3::sendSaveCalib() {
    sendImmediate("{\"type\":\"saveCalib\"}");
    Serial.println(F("[WsV3] Save calibration requested"));
}

void WsClientV3::sendLoadCalib() {
    sendImmediate("{\"type\":\"loadCalib\"}");
    Serial.println(F("[WsV3] Load calibration requested"));
}

void WsClientV3::sendSetCalibLock(bool locked) {
    char buf[64];
    snprintf(buf, sizeof(buf), "{\"type\":\"setCalibLock\",\"locked\":%s}", 
        locked ? "true" : "false");
    sendImmediate(buf);
    Serial.printf("[WsV3] Calib lock: %s\n", locked ? "ON" : "OFF");
}

void WsClientV3::sendGetServoCalib() {
    sendImmediate("{\"type\":\"getServoCalib\"}");
    Serial.println(F("[WsV3] Requesting servo calibration from bot"));
}

// =============================================================================
// Response Processing
// =============================================================================
#include <ArduinoJson.h>

void WsClientV3::processResponse(const char* json) {
    if (!json) return;
    
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.printf("[WsV3] JSON parse error: %s\n", err.c_str());
        return;
    }
    
    const char* type = doc["type"];
    if (!type) return;
    
    // Servo-Kalibrierung vom Bot empfangen
    if (strcmp(type, "servoCalib") == 0) {
        uint8_t servo = doc["servo"] | 255;
        if (servo < 8 && servoCalibCallback) {
            ServoCalibParams params;
            params.offset = doc["offset"] | 0;
            params.minAngle = doc["min"] | 20;
            params.maxAngle = doc["max"] | 160;
            params.centerAngle = doc["center"] | 90;
            params.validate();
            
            servoCalibCallback(servo, params);
            Serial.printf("[WsV3] Received servo %d calib: off=%d\n", servo, params.offset);
        }
    }
    // Alle 8 Servos als Array
    else if (strcmp(type, "allServoCalib") == 0) {
        JsonArray servos = doc["servos"];
        if (servos && servoCalibCallback) {
            for (size_t i = 0; i < servos.size() && i < 8; i++) {
                JsonObject s = servos[i];
                ServoCalibParams params;
                params.offset = s["offset"] | 0;
                params.minAngle = s["min"] | 20;
                params.maxAngle = s["max"] | 160;
                params.centerAngle = s["center"] | 90;
                params.validate();
                
                servoCalibCallback(i, params);
            }
            Serial.println(F("[WsV3] Received all servo calibrations"));
        }
    }
}
