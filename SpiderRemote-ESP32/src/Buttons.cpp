#include "Buttons.h"

void Button::begin(int pin_, uint32_t debounceMs_, uint32_t longPressMs_) {
  pin = pin_;
  debounceMs = debounceMs_;
  longPressMs = longPressMs_;
  pinMode(pin, INPUT_PULLUP);
  stable = HIGH;
  lastRead = HIGH;
  pending = PressType::None;
  pressing = false;
  longFired = false;
}

void Button::update() {
  bool r = digitalRead(pin);
  uint32_t now = millis();

  if (r != lastRead) {
    lastRead = r;
    lastChangeMs = now;
  }

  if ((now - lastChangeMs) > debounceMs && r != stable) {
    bool prev = stable;
    stable = r;

    if (prev == HIGH && stable == LOW) {
      pressing = true;
      pressStartMs = now;
      longFired = false;
    } else if (prev == LOW && stable == HIGH) {
      if (!longFired) pending = PressType::Short;
      pressing = false;
    }
  }

  if (pressing && !longFired && (now - pressStartMs) >= longPressMs) {
    longFired = true;
    pending = PressType::Long;
  }
}

PressType Button::consume() {
  PressType p = pending;
  pending = PressType::None;
  return p;
}
