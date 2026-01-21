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

  if (calibState == CalibState::WAITING) {
    u8g2.setFont(u8g2_font_7x13B_tf);
    drawCentered("KALIBRIERUNG", 16);
    u8g2.setFont(u8g2_font_6x12_tf);
    drawCentered("Controller flach", 32);
    drawCentered("auf Tisch legen!", 44);
    u8g2.setFont(u8g2_font_7x13B_tf);
    drawCentered("MID druecken", 60);
    u8g2.sendBuffer();
    return;
  }

  if (calibState == CalibState::RUNNING) {
    u8g2.setFont(u8g2_font_7x13B_tf);
    drawCentered("KALIBRIERE...", 20);
    u8g2.setFont(u8g2_font_6x12_tf);
    drawCentered("Nicht bewegen!", 36);
    u8g2.drawFrame(14, 44, 100, 12);
    u8g2.drawBox(16, 46, (calibProgress * 96) / 100, 8);
    snprintf(buf, sizeof(buf), "%d%%", calibProgress);
    drawCentered(buf, 64);
    u8g2.sendBuffer();
    return;
  }

  if (calibState == CalibState::DONE) {
    u8g2.setFont(u8g2_font_7x13B_tf);
    drawCentered("FERTIG!", 28);
    u8g2.setFont(u8g2_font_6x12_tf);
    drawCentered("Gyro-Mode aktiv", 46);
    drawCentered("Kippen = Steuern", 60);
    u8g2.sendBuffer();
    return;
  }

  if (calibState == CalibState::FAILED) {
    u8g2.setFont(u8g2_font_7x13B_tf);
    drawCentered("FEHLER!", 28);
    u8g2.setFont(u8g2_font_6x12_tf);
    drawCentered("MPU nicht bereit", 46);
    u8g2.sendBuffer();
    return;
  }

  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(0, 10, wsConnected ? "WS:OK" : "WS:--");

  snprintf(buf, sizeof(buf), "Vmax:%d Spd:%d", vmax, (currentSpeed < 0 ? 0 : currentSpeed));
  u8g2.drawStr(54, 10, buf);

  u8g2.setFont(u8g2_font_7x13B_tf);
  if (ui.mode == UiMode::DRIVE)   drawCentered("DRIVE", 28);
  if (ui.mode == UiMode::MOTION)  drawCentered("MOTION", 28);
  if (ui.mode == UiMode::CALIB)   drawCentered("CALIB", 28);
  if (ui.mode == UiMode::TERRAIN) drawCentered("TERRAIN", 28);
  if (ui.mode == UiMode::ACTION)  drawCentered("ACTION", 28);

  u8g2.setFont(u8g2_font_6x12_tf);

  if (ui.mode == UiMode::DRIVE) {
    snprintf(buf, sizeof(buf), "Move: %s", (currentMove && currentMove[0] ? currentMove : "STOP"));
    u8g2.drawStr(0, 46, buf);
    snprintf(buf, sizeof(buf), "Terrain: %s", ui.terrainActive);
    u8g2.drawStr(0, 60, buf);

    u8g2.setFont(u8g2_font_5x8_tf);
    u8g2.drawStr(0, 38, "Hold MID: Menu | STOP: Failsafe");
  } else if (ui.mode == UiMode::MOTION) {
    if (!motionAvailable) {
      drawCentered("MPU nicht gefunden!", 46);
    } else if (!motionCalibrated) {
      drawCentered("Nicht kalibriert", 40);
      u8g2.setFont(u8g2_font_5x8_tf);
      u8g2.drawStr(0, 56, "Hold MID: Kalibrieren");
    } else {
      snprintf(buf, sizeof(buf), "Move: %s", (currentMove && currentMove[0] ? currentMove : "STOP"));
      u8g2.drawStr(0, 46, buf);
      u8g2.setFont(u8g2_font_5x8_tf);
      u8g2.drawStr(0, 38, "Kippen=Steuern | MID=Kalib");
      u8g2.drawStr(0, 60, "Drehen=Wenden");
    }
  } else if (ui.mode == UiMode::CALIB) {
    if (inputCalibStep == InputCalibStep::IDLE) {
      drawCentered("MID: Start Kalib.", 46);
      u8g2.setFont(u8g2_font_5x8_tf);
      u8g2.drawStr(0, 60, "Hold MID: naechster Modus");
    } else if (inputCalibStep == InputCalibStep::DEADBAND) {
      snprintf(buf, sizeof(buf), "Deadband: %d%%", deadbandPercent);
      drawCentered(buf, 42);
      u8g2.setFont(u8g2_font_5x8_tf);
      u8g2.drawStr(0, 56, "L/R anpassen | MID speichern");
    } else if (inputCalibStep == InputCalibStep::DONE) {
      drawCentered("Gespeichert!", 46);
    } else if (inputCalibText) {
      const char* line1 = inputCalibText;
      const char* line2 = strchr(inputCalibText, '\n');
      if (line2) {
        int len1 = line2 - line1;
        char tmp[32];
        strncpy(tmp, line1, len1);
        tmp[len1] = '\0';
        drawCentered(tmp, 42);
        drawCentered(line2 + 1, 56);
      } else {
        drawCentered(inputCalibText, 48);
      }
    }
  } else if (ui.mode == UiMode::TERRAIN) {
    snprintf(buf, sizeof(buf), "Aktiv: %s", ui.terrainActive);
    u8g2.drawStr(0, 40, buf);
    snprintf(buf, sizeof(buf), "> %s", TERRAIN_LABELS[ui.terrainIndex]);
    u8g2.drawStr(0, 56, buf);

    u8g2.setFont(u8g2_font_5x8_tf);
    u8g2.drawStr(0, 38, "L/R waehlen | MID bestaetigen");
  } else if (ui.mode == UiMode::ACTION) {
    snprintf(buf, sizeof(buf), "> %s", ACTION_LABELS[ui.actionIndex]);
    u8g2.drawStr(0, 56, buf);

    u8g2.setFont(u8g2_font_5x8_tf);
    u8g2.drawStr(0, 38, "L/R waehlen | MID senden");
  }

  u8g2.sendBuffer();
}
