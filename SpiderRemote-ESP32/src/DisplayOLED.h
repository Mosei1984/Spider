#pragma once
#include <Arduino.h>
#include <U8g2lib.h>
#include "UiMenu.h"

class DisplayOLED {
public:
  DisplayOLED();
  void begin(uint32_t updateMs);
  void tick(bool wsConnected,
            int vmax,
            int currentSpeed,
            const char* currentMove,
            const UiState& ui);

private:
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
  uint32_t updateMs = 50;
  uint32_t lastMs = 0;

  void drawCentered(const char* text, int y);
};
