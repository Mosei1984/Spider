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
                       const UiState& ui) {
  uint32_t now = millis();
  if (now - lastMs < updateMs) return;
  lastMs = now;

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);

  u8g2.drawStr(0, 10, wsConnected ? "WS:OK" : "WS:--");

  char buf[32];
  snprintf(buf, sizeof(buf), "Vmax:%d Spd:%d", vmax, (currentSpeed < 0 ? 0 : currentSpeed));
  u8g2.drawStr(54, 10, buf);

  u8g2.setFont(u8g2_font_7x13B_tf);
  if (ui.mode == UiMode::DRIVE)   drawCentered("DRIVE", 28);
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
