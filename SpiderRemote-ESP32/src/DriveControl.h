#pragma once
#include <Arduino.h>
#include "WsClient.h"
#include "Inputs.h"

enum class MoveDir : uint8_t {
  None = 0,
  Forward,
  Backward,
  Left,
  Right,
  TurnLeft,
  TurnRight
};

const char* moveDirToString(MoveDir dir);

class DriveControl {
public:
  void begin(WsClient* ws);
  void tick(const InputState& in, int speedMin);

  const char* currentMove() const { return moveDirToString(move); }
  int currentSpeed() const { return speed; }

  void forceStop(); // STOP button

private:
  WsClient* ws = nullptr;

  MoveDir move = MoveDir::None;
  int speed = -1;
  bool moving = false;

  void apply(MoveDir desiredMove, int desiredSpeed, int speedMin);
};
