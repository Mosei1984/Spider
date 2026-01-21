#include "UiMenu.h"

void UiMenu::begin(WsClient* ws_) { ws = ws_; }

void UiMenu::tick(Button& left, Button& mid, Button& right, UiState& st) {
  if (!ws) return;

  PressType pL = left.consume();
  PressType pM = mid.consume();
  PressType pR = right.consume();

  if (pM == PressType::Long) {
    if (st.mode == UiMode::DRIVE) st.mode = UiMode::TERRAIN;
    else if (st.mode == UiMode::TERRAIN) st.mode = UiMode::ACTION;
    else st.mode = UiMode::DRIVE;
    return;
  }

  if (st.mode == UiMode::DRIVE) {
    return;
  }

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
    } else if (st.mode == UiMode::ACTION) {
      ws->sendCmd(ACTION_ITEMS[st.actionIndex]);
    }
  }
}
