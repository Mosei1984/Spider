#include "DisplayOLED.h"
#include "Config.h"

DisplayOLED::DisplayOLED()
: u8g2(U8G2_R0, U8X8_PIN_NONE) {}

void DisplayOLED::begin(uint32_t updateMs_) {
  updateMs = updateMs_;
  u8g2.begin();
  u8g2.setContrast(OLED_CONTRAST);
}

void DisplayOLED::drawCentered(const char* text, int y) {
  int w = u8g2.getStrWidth(text);
  int x = (128 - w) / 2;
  u8g2.drawStr(x, y, text);
}

void DisplayOLED::drawRightAligned(const char* text, int y) {
  int w = u8g2.getStrWidth(text);
  u8g2.drawStr(128 - w, y, text);
}

void DisplayOLED::tick(bool wsConnected,
                       int vmax,
                       int currentSpeed,
                       const char* currentMove,
                       const UiState& ui,
                       bool motionAvailable,
                       bool motionCalibrated,
                       CalibState calibState,
                       int calibProgress,
                       InputCalibStep inputCalibStep,
                       const char* inputCalibText,
                       int deadbandPercent) {
  uint32_t now = millis();
  if (now - lastMs < updateMs) return;
  lastMs = now;

  u8g2.clearBuffer();
  char buf[32];

  // ===== GYRO CALIBRATION SCREENS (volle Übernahme) =====
  if (calibState == CalibState::WAITING) {
    u8g2.setFont(u8g2_font_6x12_tf);
    drawCentered("GYRO KALIB.", 12);
    drawCentered("Controller flach", 28);
    drawCentered("auf Tisch legen", 40);
    u8g2.setFont(u8g2_font_5x8_tf);
    drawCentered("[MID] Start", 58);
    u8g2.sendBuffer();
    return;
  }

  if (calibState == CalibState::RUNNING) {
    u8g2.setFont(u8g2_font_6x12_tf);
    drawCentered("KALIBRIERE...", 14);
    u8g2.setFont(u8g2_font_5x8_tf);
    drawCentered("Nicht bewegen!", 28);
    u8g2.drawFrame(14, 34, 100, 10);
    u8g2.drawBox(16, 36, (calibProgress * 96) / 100, 6);
    snprintf(buf, sizeof(buf), "%d%%", calibProgress);
    drawCentered(buf, 58);
    u8g2.sendBuffer();
    return;
  }

  if (calibState == CalibState::DONE) {
    u8g2.setFont(u8g2_font_6x12_tf);
    drawCentered("FERTIG!", 24);
    u8g2.setFont(u8g2_font_5x8_tf);
    drawCentered("Gyro aktiv", 40);
    drawCentered("Kippen = Steuern", 52);
    u8g2.sendBuffer();
    return;
  }

  if (calibState == CalibState::FAILED) {
    u8g2.setFont(u8g2_font_6x12_tf);
    drawCentered("FEHLER!", 24);
    u8g2.setFont(u8g2_font_5x8_tf);
    drawCentered("MPU nicht bereit", 40);
    u8g2.sendBuffer();
    return;
  }

  // ===== STATUS BAR (Zeile 1: 0-10px) =====
  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.drawStr(0, 7, wsConnected ? "WS" : "--");
  
  snprintf(buf, sizeof(buf), "S:%d", vmax);
  drawRightAligned(buf, 7);

  // Mode in der Mitte der Statuszeile
  const char* modeName = "";
  switch (ui.mode) {
    case UiMode::DRIVE:   modeName = "DRIVE"; break;
    case UiMode::MOTION:  modeName = "MOTION"; break;
    case UiMode::CALIB:   modeName = "CALIB"; break;
    case UiMode::TERRAIN: modeName = "TERRAIN"; break;
    case UiMode::ACTION:  modeName = "ACTION"; break;
  }
  drawCentered(modeName, 7);

  // Trennlinie
  u8g2.drawHLine(0, 9, 128);

  // ===== CONTENT AREA (12-54px) =====
  
  if (ui.mode == UiMode::DRIVE) {
    u8g2.setFont(u8g2_font_6x12_tf);
    snprintf(buf, sizeof(buf), "%s", (currentMove && currentMove[0] ? currentMove : "STOP"));
    drawCentered(buf, 28);
    
    u8g2.setFont(u8g2_font_5x8_tf);
    snprintf(buf, sizeof(buf), "Terrain: %s", ui.terrainActive);
    drawCentered(buf, 42);
  } 
  
  else if (ui.mode == UiMode::MOTION) {
    if (!motionAvailable) {
      u8g2.setFont(u8g2_font_5x8_tf);
      drawCentered("MPU nicht gefunden!", 28);
    } else if (!motionCalibrated) {
      u8g2.setFont(u8g2_font_5x8_tf);
      drawCentered("Nicht kalibriert", 28);
      drawCentered("[MID] Kalibrieren", 42);
    } else {
      u8g2.setFont(u8g2_font_6x12_tf);
      snprintf(buf, sizeof(buf), "%s", (currentMove && currentMove[0] ? currentMove : "STOP"));
      drawCentered(buf, 28);
      u8g2.setFont(u8g2_font_5x8_tf);
      drawCentered("Kippen=Fahren Drehen=Turn", 42);
    }
  } 
  
  else if (ui.mode == UiMode::CALIB) {
    if (inputCalibStep == InputCalibStep::IDLE) {
      u8g2.setFont(u8g2_font_5x8_tf);
      drawCentered("Joystick Kalibrierung", 24);
      drawCentered("[MID] Start", 38);
    } else if (inputCalibStep == InputCalibStep::DEADBAND) {
      u8g2.setFont(u8g2_font_6x12_tf);
      snprintf(buf, sizeof(buf), "Deadband: %d%%", deadbandPercent);
      drawCentered(buf, 28);
      u8g2.setFont(u8g2_font_5x8_tf);
      drawCentered("[L/R] +/- [MID] OK", 42);
    } else if (inputCalibStep == InputCalibStep::DONE) {
      u8g2.setFont(u8g2_font_6x12_tf);
      drawCentered("Gespeichert!", 32);
    } else if (inputCalibText) {
      u8g2.setFont(u8g2_font_5x8_tf);
      const char* line2 = strchr(inputCalibText, '\n');
      if (line2) {
        int len1 = line2 - inputCalibText;
        char tmp[32];
        if (len1 > 31) len1 = 31;
        strncpy(tmp, inputCalibText, len1);
        tmp[len1] = '\0';
        drawCentered(tmp, 26);
        drawCentered(line2 + 1, 38);
      } else {
        drawCentered(inputCalibText, 32);
      }
    }
  } 
  
  else if (ui.mode == UiMode::TERRAIN) {
    u8g2.setFont(u8g2_font_5x8_tf);
    snprintf(buf, sizeof(buf), "Aktiv: %s", ui.terrainActive);
    drawCentered(buf, 22);
    
    u8g2.setFont(u8g2_font_6x12_tf);
    snprintf(buf, sizeof(buf), "> %s", TERRAIN_LABELS[ui.terrainIndex]);
    drawCentered(buf, 38);
  } 
  
  else if (ui.mode == UiMode::ACTION) {
    u8g2.setFont(u8g2_font_6x12_tf);
    snprintf(buf, sizeof(buf), "%s", ACTION_LABELS[ui.actionIndex]);
    drawCentered(buf, 32);
  }

  // ===== FOOTER (56-64px) =====
  u8g2.drawHLine(0, 54, 128);
  u8g2.setFont(u8g2_font_5x8_tf);
  
  if (ui.mode == UiMode::DRIVE || ui.mode == UiMode::MOTION) {
    drawCentered("[HOLD MID] Menu", 63);
  } else if (ui.mode == UiMode::TERRAIN || ui.mode == UiMode::ACTION) {
    drawCentered("[L/R] Wahl [MID] OK", 63);
  } else if (ui.mode == UiMode::CALIB && inputCalibStep == InputCalibStep::IDLE) {
    drawCentered("[HOLD MID] Weiter", 63);
  }

  u8g2.sendBuffer();
}

// =============================================================================
// v3: Einfache 4-Zeilen Anzeige für v3-Menüs
// =============================================================================
void DisplayOLED::tickV3(bool wsConnected, int speed,
                         const char* line1, const char* line2,
                         const char* line3, const char* line4,
                         int progressBar) {
  uint32_t now = millis();
  if (now - lastMs < updateMs) return;
  lastMs = now;

  u8g2.clearBuffer();
  char buf[32];

  // Status Bar (Zeile 1)
  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.drawStr(0, 7, wsConnected ? "WS" : "--");
  snprintf(buf, sizeof(buf), "S:%d", speed);
  drawRightAligned(buf, 7);
  
  // Trennlinie
  u8g2.drawHLine(0, 9, 128);

  // Content: 4 Zeilen
  u8g2.setFont(u8g2_font_6x12_tf);
  if (line1 && line1[0]) drawCentered(line1, 22);
  
  u8g2.setFont(u8g2_font_5x8_tf);
  if (line2 && line2[0]) drawCentered(line2, 34);
  
  // Fortschrittsbalken oder line3
  if (progressBar >= 0) {
    // Rahmen
    u8g2.drawFrame(14, 38, 100, 10);
    // Füllung
    int fillWidth = (progressBar * 96) / 100;
    if (fillWidth > 0) {
      u8g2.drawBox(16, 40, fillWidth, 6);
    }
  } else {
    if (line3 && line3[0]) drawCentered(line3, 44);
  }
  
  // Footer
  u8g2.drawHLine(0, 54, 128);
  if (line4 && line4[0]) drawCentered(line4, 63);

  u8g2.sendBuffer();
}
