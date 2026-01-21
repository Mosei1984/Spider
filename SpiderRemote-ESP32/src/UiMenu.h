#pragma once
#include <Arduino.h>
#include "Buttons.h"
#include "WsClient.h"
#include "Config.h"

enum class UiMode { DRIVE, MOTION, CALIB, TERRAIN, ACTION };

struct UiState {
  UiMode mode = UiMode::DRIVE;
  int terrainIndex = 0;
  int actionIndex  = 3; // default: hello
  const char* terrainActive = "normal";
};

class UiMenu {
public:
  void begin(WsClient* ws);
  void tick(PressType pL, PressType pM, PressType pR, UiState& st);

private:
  WsClient* ws = nullptr;
};
