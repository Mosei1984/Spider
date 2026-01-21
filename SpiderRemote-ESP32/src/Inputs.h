#pragma once
#include <Arduino.h>

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

private:
  int pJoyX=0, pJoyY=0, pVmax=0, pTurn=0;
  int adcMax=4095;

  float normCentered(int raw) const;
  int mapSpeed(int raw, int outMin, int outMax) const;
};
