// =============================================================================
// main_v3.cpp - Hauptdatei für v3 Spider Controller
// =============================================================================
// ESP8266 Spider Controller mit erweiterter Gait-Runtime
// Features:
//   - Stride-Skalierung (Hip stärker, Knee moderater)
//   - Micro-Stepping / feinere Interpolation
//   - Timing-Shaping (Swing schneller, Stance langsamer)
//   - Soft-Start/Stop Ramping
//   - Servo-Kalibrierung mit Limits
// =============================================================================
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <Servo.h>

// v3 Module
#include "robot/RobotController_v3.h"
#include "motion/MotionData_v3.h"
#include "gait/GaitRuntime.h"
#include "calibration/ServoCalibration.h"
#include "web/WebServer_v3.h"

// =============================================================================
// WiFi-Konfiguration
// =============================================================================
// Heim-WLAN
const char* HOME_SSID = "";
const char* HOME_PASSWORD = "";

// Access Point (Fallback)
const char* AP_SSID = "QuadBot-E";
const char* AP_PASSWORD = "12345678";

// =============================================================================
// Setup
// =============================================================================
void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println(F("\n\n========================================"));
    Serial.println(F("  Spider Controller v3 - Starting..."));
    Serial.println(F("========================================\n"));
    
    // LittleFS initialisieren
    if (!LittleFS.begin()) {
        Serial.println(F("[FS] LittleFS mount failed!"));
    } else {
        Serial.println(F("[FS] LittleFS mounted"));
    }
    
    // Servos initialisieren
    Serial.println(F("[Servo] Initializing..."));
    servo_14.attach(14, SERVOMIN, SERVOMAX);
    servo_12.attach(12, SERVOMIN, SERVOMAX);
    servo_13.attach(13, SERVOMIN, SERVOMAX);
    servo_15.attach(15, SERVOMIN, SERVOMAX);
    servo_16.attach(16, SERVOMIN, SERVOMAX);
    servo_5.attach(5, SERVOMIN, SERVOMAX);
    servo_4.attach(4, SERVOMIN, SERVOMAX);
    servo_2.attach(2, SERVOMIN, SERVOMAX);
    
    // Servo-Kalibrierung laden
    ServoCalibration::init();
    
    // GaitRuntime initialisieren
    GaitRuntime::init();
    
    // Servos in Startposition
    Servo_PROGRAM_Zero();
    Serial.println(F("[Servo] Ready"));
    
    // WiFi verbinden - erst Heim-WLAN, dann AP-Fallback
    Serial.printf("[WiFi] Connecting to %s", HOME_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(HOME_SSID, HOME_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 60) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(F(" Connected!"));
        Serial.printf("[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println(F(" Failed!"));
        Serial.printf("[WiFi] Starting AP mode: %s\n", AP_SSID);
        WiFi.mode(WIFI_AP);
        WiFi.softAP(AP_SSID, AP_PASSWORD, 5);  // Kanal 5
        Serial.printf("[WiFi] AP IP: %s\n", WiFi.softAPIP().toString().c_str());
    }
    
    // WebServer starten
    setupWebServer();
    
    Serial.println(F("\n========================================"));
    Serial.println(F("  Spider Controller v3 - Ready!"));
    Serial.println(F("========================================\n"));
}

// =============================================================================
// Loop
// =============================================================================
void loop() {
    // Robot Controller verarbeiten (nicht-blockierend)
    robotController.processQueue();
    
    // Watchdog füttern
    yield();
}
