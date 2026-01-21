#pragma once
#include <Arduino.h>
#include "Buttons.h"
#include "WsClient.h"
#include "Config.h"

enum class UiMode { DRIVE, TERRAIN, ACTION };

struct UiState {
  UiMode mode = UiMode::DRIVE;
  int terrainIndex = 0;
  int actionIndex  = 3; // default: hello
  const char* terrainActive = "normal";
};

class UiMenu {
public:
  void begin(WsClient* ws);
  void tick(Button& left, Button& mid, Button& right, UiState& st);

private:
  WsClient* ws = nullptr;
};
