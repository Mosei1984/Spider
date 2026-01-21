#pragma once
#include <Arduino.h>
#include "Credentials.h"  // WLAN Credentials (nicht in Git!)

// ========= DEBUG MODE =========
// Set to false for normal operation with joystick hardware
// Set to true for testing via Serial Monitor (no hardware needed)
#define SERIAL_CMD_MODE false

// ========= WLAN Timeouts =========
static constexpr uint32_t WIFI_AP_TIMEOUT_MS   = 8000;   // 8s für AP
static constexpr uint32_t WIFI_HOME_TIMEOUT_MS = 10000;  // 10s für Home

static const uint16_t SPIDER_PORT = 80;
static const char* SPIDER_PATH = "/ws";

// ========= OLED =========
// Standard ESP32 I2C Pins: SDA=21, SCL=22
static constexpr int OLED_SDA = 21;
static constexpr int OLED_SCL = 22;

// ========= Analog Pins (ADC1) =========
static constexpr int PIN_JOY_X    = 34;
static constexpr int PIN_JOY_Y    = 35;
static constexpr int PIN_POT_VMAX = 32;
static constexpr int PIN_POT_TURN = 33;

// ========= Buttons =========
// 3-Button menu
static constexpr int PIN_BTN_LEFT  = 25;
static constexpr int PIN_BTN_MID   = 26;
static constexpr int PIN_BTN_RIGHT = 27;

// 4th button: STOP (always high priority)
static constexpr int PIN_BTN_STOP  = 14;

// ========= Tuning =========
static constexpr int   ADC_MAX = 4095;

static constexpr float DEAD_JOY  = 0.08f;
static constexpr float DEAD_TURN = 0.06f;

static constexpr uint32_t MIN_SEND_INTERVAL_MS = 50;   // matches Web-UI
static constexpr uint32_t BTN_DEBOUNCE_MS      = 30;
static constexpr uint32_t BTN_LONGPRESS_MS     = 700;

static constexpr int SPEED_MIN = 10;
static constexpr int SPEED_MAX = 100;

// ========= Motion (MPU6050) =========
static constexpr float DEAD_TILT        = 0.20f;  // Größere Totzone für Neigung
static constexpr float DEAD_TURN_MOTION = 0.15f;  // Größere Totzone für Drehung
static constexpr float TILT_MAX_DEG     = 25.0f;
static constexpr float TURN_MAX_DPS     = 150.0f;
static constexpr float MOTION_ALPHA     = 0.98f;

static constexpr uint32_t OLED_UPDATE_MS = 50; // 20 Hz

// ========= OLED =========
static constexpr uint8_t OLED_CONTRAST = 255;

// ========= WiFi Reconnect =========
static constexpr uint32_t WIFI_RECONNECT_INTERVAL_MS = 5000;

// ========= Menu lists =========
static constexpr const char* TERRAIN_ITEMS[]  = {"normal","uphill","downhill"};
static constexpr const char* TERRAIN_LABELS[] = {"Flach","Bergauf","Bergab"};
static constexpr int TERRAIN_COUNT = sizeof(TERRAIN_ITEMS) / sizeof(TERRAIN_ITEMS[0]);
static_assert(sizeof(TERRAIN_ITEMS)/sizeof(*TERRAIN_ITEMS) == 
              sizeof(TERRAIN_LABELS)/sizeof(*TERRAIN_LABELS), "Terrain arrays mismatch");

static constexpr const char* ACTION_ITEMS[] = {
  "standby","sleep","lie","hello","pushup","fighting","dance1","dance2","dance3"
};
static constexpr const char* ACTION_LABELS[] = {
  "Stehen","Schlafen","Hinlegen","Winken","Liegestuetz","Kaempfen","Tanz 1","Tanz 2","Tanz 3"
};
static constexpr int ACTION_COUNT = sizeof(ACTION_ITEMS) / sizeof(ACTION_ITEMS[0]);
static_assert(sizeof(ACTION_ITEMS)/sizeof(*ACTION_ITEMS) == 
              sizeof(ACTION_LABELS)/sizeof(*ACTION_LABELS), "Action arrays mismatch");
