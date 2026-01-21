#pragma once
#include <Arduino.h>

enum class PressType { None, Short, Long };

class Button {
public:
  void begin(int pin, uint32_t debounceMs, uint32_t longPressMs);
  void update();
  PressType consume(); // once per press

  bool isDown() const { return stable == LOW; }

private:
  int pin=0;
  uint32_t debounceMs=30, longPressMs=700;

  bool stable=HIGH, lastRead=HIGH;
  uint32_t lastChangeMs=0;

  bool pressing=false;
  uint32_t pressStartMs=0;
  bool longFired=false;

  PressType pending = PressType::None;
};
