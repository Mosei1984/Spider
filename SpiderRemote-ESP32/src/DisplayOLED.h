#pragma once
#include <Arduino.h>
#include <U8g2lib.h>
#include "UiMenu.h"
#include "MotionInputs.h"
#include "InputCalib.h"

class DisplayOLED {
public:
  DisplayOLED();
  void begin(uint32_t updateMs);
  void tick(bool wsConnected,
            int vmax,
            int currentSpeed,
            const char* currentMove,
            const UiState& ui,
            bool motionAvailable = false,
            bool motionCalibrated = false,
            CalibState calibState = CalibState::IDLE,
            int calibProgress = 0,
            InputCalibStep inputCalibStep = InputCalibStep::IDLE,
            const char* inputCalibText = nullptr,
            int deadbandPercent = 5);

  // v3: Direkte Anzeige von 4 Textzeilen
  void tickV3(bool wsConnected, int speed,
              const char* line1, const char* line2,
              const char* line3, const char* line4,
              int progressBar = -1);  // -1 = kein Balken, 0-100 = Fortschritt

private:
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
  uint32_t updateMs = 50;
  uint32_t lastMs = 0;

  void drawCentered(const char* text, int y);
  void drawRightAligned(const char* text, int y);
};
