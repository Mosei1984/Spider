#include "UiMenu.h"

void UiMenu::begin(WsClient* ws_) { ws = ws_; }

void UiMenu::tick(PressType pL, PressType pM, PressType pR, UiState& st) {
  if (!ws) return;

  // Navigation nur in TERRAIN und ACTION modes
  if (st.mode == UiMode::TERRAIN || st.mode == UiMode::ACTION) {
    if (pL == PressType::Short) {
      if (st.mode == UiMode::TERRAIN) st.terrainIndex = (st.terrainIndex - 1 + TERRAIN_COUNT) % TERRAIN_COUNT;
      if (st.mode == UiMode::ACTION)  st.actionIndex  = (st.actionIndex - 1 + ACTION_COUNT) % ACTION_COUNT;
    }
    if (pR == PressType::Short) {
      if (st.mode == UiMode::TERRAIN) st.terrainIndex = (st.terrainIndex + 1) % TERRAIN_COUNT;
      if (st.mode == UiMode::ACTION)  st.actionIndex  = (st.actionIndex + 1) % ACTION_COUNT;
    }

    if (pM == PressType::Short) {
      if (st.mode == UiMode::TERRAIN) {
        ws->sendTerrain(TERRAIN_ITEMS[st.terrainIndex]);
        st.terrainActive = TERRAIN_ITEMS[st.terrainIndex];
        Serial.printf("[Menu] Terrain: %s\n", TERRAIN_ITEMS[st.terrainIndex]);
      } else if (st.mode == UiMode::ACTION) {
        ws->sendCmd(ACTION_ITEMS[st.actionIndex]);
        Serial.printf("[Menu] Action: %s\n", ACTION_ITEMS[st.actionIndex]);
      }
    }
  }
}
