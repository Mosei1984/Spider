#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include "web/WebServer.h"
#include "robot/RobotController.h"
#include "wifi/WiFiManager.h"

// ===== WIFI KONFIGURATION =====
// Heim-WLAN (leer lassen = nur AP-Mode)
const char* HOME_SSID = "";           // z.B. "MeinWLAN"
const char* HOME_PASSWORD = "";       // z.B. "MeinPasswort"

// Access Point (Fallback)
const char* AP_SSID = "QuadBot-E";
const char* AP_PASSWORD = "12345678";
// ==============================

void setup() {
    Serial.setTimeout(10);
    Serial.begin(115200);
    Serial.println("\n\n=== Spider Bot ===");
    
    // LittleFS früh initialisieren für Offsets
    if (!LittleFS.begin()) {
        Serial.println("[FS] LittleFS mount failed!");
    } else {
        Serial.println("[FS] LittleFS mounted");
        loadOffsetsFromFile();  // VOR Servo_PROGRAM_Zero!
    }
    
    // WiFi konfigurieren
    WiFiConfig wifiConfig = {
        HOME_SSID,      // Heim-WLAN
        HOME_PASSWORD,
        AP_SSID,        // AP-Fallback
        AP_PASSWORD,
        5,              // AP-Kanal
        30000           // 15s Timeout für STA
    };
    wifiManager.setConfig(wifiConfig);
    wifiManager.begin();
    
    Serial.printf("Mode: %s | IP: %s\n", 
        wifiManager.isStationMode() ? "STATION" : "AP",
        wifiManager.getIP().toString().c_str());
    
    servo_14.attach(14, SERVOMIN, SERVOMAX);
    servo_12.attach(12, SERVOMIN, SERVOMAX);
    servo_13.attach(13, SERVOMIN, SERVOMAX);
    servo_15.attach(15, SERVOMIN, SERVOMAX);
    servo_16.attach(16, SERVOMIN, SERVOMAX);
    servo_5.attach(5, SERVOMIN, SERVOMAX);
    servo_4.attach(4, SERVOMIN, SERVOMAX);
    servo_2.attach(2, SERVOMIN, SERVOMAX);
    
    Servo_PROGRAM_Zero();  // Jetzt mit geladenen Offsets
    setupWebServer();      // Ohne loadOffsetsFromFile
}

void loop() {
    robotController.processQueue();
    yield();
}
