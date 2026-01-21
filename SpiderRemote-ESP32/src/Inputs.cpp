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

int Inputs::readFiltered(int pin, int* buf) {
  int raw = analogRead(pin);
  buf[bufIdx] = raw;
  
  int count = bufFilled ? FILTER_SIZE : (bufIdx + 1);
  long sum = 0;
  for (int i = 0; i < count; i++) {
    sum += buf[i];
  }
  return (int)(sum / count);
}

InputState Inputs::read(float deadJoy, float deadTurn, int speedMin, int speedMax_) {
  InputState s{};
  if (speedMin > speedMax_) speedMax_ = speedMin;
  
  int rawV = readFiltered(pVmax, bufVmax);
  int rawT = readFiltered(pTurn, bufTurn);
  int rawX = readFiltered(pJoyX, bufJoyX);
  int rawY = readFiltered(pJoyY, bufJoyY);

  bufIdx = (bufIdx + 1) % FILTER_SIZE;
  if (bufIdx == 0) bufFilled = true;

  s.speedMax = mapSpeed(rawV, speedMin, speedMax_);
  s.turn = normCentered(rawT);
  s.joyX = normCentered(rawX);
  s.joyY = normCentered(rawY);

  if (fabs(s.turn) < deadTurn) s.turn = 0;
  if (fabs(s.joyX) < deadJoy)  s.joyX = 0;
  if (fabs(s.joyY) < deadJoy)  s.joyY = 0;

  return s;
}

int Inputs::readSpeedPot(int speedMin, int speedMax_) {
  int rawV = readFiltered(pVmax, bufVmax);
  bufIdx = (bufIdx + 1) % FILTER_SIZE;
  if (bufIdx == 0) bufFilled = true;
  return mapSpeed(rawV, speedMin, speedMax_);
}

void Inputs::readRaw(int& joyX, int& joyY, int& turn, int& vmax) {
  joyX = readFiltered(pJoyX, bufJoyX);
  joyY = readFiltered(pJoyY, bufJoyY);
  turn = readFiltered(pTurn, bufTurn);
  vmax = readFiltered(pVmax, bufVmax);
  bufIdx = (bufIdx + 1) % FILTER_SIZE;
  if (bufIdx == 0) bufFilled = true;
}

InputState Inputs::readCalibrated(const InputCalibData& calib, int speedMin, int speedMax_) {
  InputState s{};
  if (speedMin > speedMax_) speedMax_ = speedMin;

  int rawX, rawY, rawT, rawV;
  readRaw(rawX, rawY, rawT, rawV);

  auto normAxis = [](int raw, const AxisCalib& cal) -> float {
    int center = cal.rawCenter;
    int half = (cal.rawMax - cal.rawMin) / 2;
    if (half < 1) half = 1;
    float v = (float)(raw - center) / half;
    if (v < -1.0f) v = -1.0f;
    if (v > 1.0f) v = 1.0f;
    float dead = (float)cal.deadband / half;
    if (fabsf(v) < dead) return 0.0f;
    float sign = (v > 0) ? 1.0f : -1.0f;
    return sign * (fabsf(v) - dead) / (1.0f - dead);
  };

  auto normLinear = [](int raw, const AxisCalib& cal, int outMin, int outMax) -> int {
    int range = cal.rawMax - cal.rawMin;
    if (range < 1) range = 1;
    long v = outMin + (long)(raw - cal.rawMin) * (outMax - outMin) / range;
    if (v < outMin) v = outMin;
    if (v > outMax) v = outMax;
    return (int)v;
  };

  s.joyX = normAxis(rawX, calib.joyX);
  s.joyY = normAxis(rawY, calib.joyY);
  s.turn = normAxis(rawT, calib.turn);
  s.speedMax = normLinear(rawV, calib.vmax, speedMin, speedMax_);

  return s;
}
