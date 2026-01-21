#include "Inputs.h"

static inline float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void Inputs::begin(int pinJoyX, int pinJoyY, int pinVmax, int pinTurn, int adcMax_) {
  pJoyX = pinJoyX; pJoyY = pinJoyY; pVmax = pinVmax; pTurn = pinTurn;
  adcMax = (adcMax_ < 2) ? 4095 : adcMax_;
}

float Inputs::normCentered(int raw) const {
  float x = (raw - (adcMax/2)) / float(adcMax/2);
  return clampf(x, -1.0f, 1.0f);
}

int Inputs::mapSpeed(int raw, int outMin, int outMax) const {
  long v = outMin + (long)raw * (outMax - outMin) / adcMax;
  if (v < outMin) v = outMin;
  if (v > outMax) v = outMax;
  return (int)v;
}

InputState Inputs::read(float deadJoy, float deadTurn, int speedMin, int speedMax_) {
  InputState s{};
  if (speedMin > speedMax_) speedMax_ = speedMin;
  
  int rawV = analogRead(pVmax);
  int rawT = analogRead(pTurn);
  int rawX = analogRead(pJoyX);
  int rawY = analogRead(pJoyY);

  s.speedMax = mapSpeed(rawV, speedMin, speedMax_);
  s.turn = normCentered(rawT);
  s.joyX = normCentered(rawX);
  s.joyY = normCentered(rawY);

  if (fabs(s.turn) < deadTurn) s.turn = 0;
  if (fabs(s.joyX) < deadJoy)  s.joyX = 0;
  if (fabs(s.joyY) < deadJoy)  s.joyY = 0;

  return s;
}
