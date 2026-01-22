#pragma once
#include <Arduino.h>
#include "InputCalib.h"

struct InputState {
  int speedMax;   // 10..100
  float joyX;     // -1..+1
  float joyY;     // -1..+1
  float turn;     // -1..+1
};

class Inputs {
public:
  void begin(int pinJoyX, int pinJoyY, int pinVmax, int pinTurn, int adcMax);
  InputState read(float deadJoy, float deadTurn, int speedMin, int speedMax);
  InputState readCalibrated(const InputCalibData& calib, int speedMin, int speedMax);
  int readSpeedPot(int speedMin, int speedMax);
  int readSpeedPotRaw(int speedMin, int speedMax);  // Ohne Filter, für Loop-Aufruf
  void readRaw(int& joyX, int& joyY, int& turn, int& vmax);

private:
  int pJoyX=0, pJoyY=0, pVmax=0, pTurn=0;
  int adcMax=4095;

  static constexpr int FILTER_SIZE = 5;
  int bufJoyX[FILTER_SIZE] = {0};
  int bufJoyY[FILTER_SIZE] = {0};
  int bufVmax[FILTER_SIZE] = {0};
  int bufTurn[FILTER_SIZE] = {0};
  int bufIdx = 0;
  bool bufFilled = false;

  int readFiltered(int pin, int* buf);
  float normCentered(int raw) const;
  int mapSpeed(int raw, int outMin, int outMax) const;
};
