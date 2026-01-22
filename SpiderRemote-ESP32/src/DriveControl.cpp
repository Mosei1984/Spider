#include "DriveControl.h"

const char* moveDirToString(MoveDir dir) {
  switch (dir) {
    case MoveDir::Forward:   return "forward";
    case MoveDir::Backward:  return "backward";
    case MoveDir::Left:      return "left";
    case MoveDir::Right:     return "right";
    case MoveDir::TurnLeft:  return "turnleft";
    case MoveDir::TurnRight: return "turnright";
    default:                 return "";
  }
}

void DriveControl::begin(WsClient* ws_) { ws = ws_; }

void DriveControl::forceStop() {
  if (!ws) return;
  ws->sendStop();
  ws->sendMoveStop();
  moving = false;
  move = MoveDir::None;
  speed = -1;
}

void DriveControl::apply(MoveDir desiredMove, int desiredSpeed, int speedMin) {
  if (!ws) return;

  // Clamp immer zuerst
  if (desiredSpeed < speedMin) desiredSpeed = speedMin;

  // Speed IMMER aktualisieren, auch im Stillstand (für nächste Bewegung)
  if (desiredSpeed != speed) {
    ws->sendSetSpeed(desiredSpeed);
    speed = desiredSpeed;
  }

  if (desiredMove == MoveDir::None) {
    if (moving) {
      ws->sendMoveStop();
      moving = false;
      move = MoveDir::None;
      speed = -1;  // Cache invalidieren - erzwingt setSpeed beim nächsten Start
    }
    return;
  }

  if (!moving || desiredMove != move) {
    ws->sendMoveStart(moveDirToString(desiredMove));
    moving = true;
    move = desiredMove;
  }
}

void DriveControl::tick(const InputState& in, int speedMin) {
  MoveDir desiredMove = MoveDir::None;

  // Priorität: Turn > Joystick Y > Joystick X
  if (in.turn != 0) {
    desiredMove = (in.turn > 0) ? MoveDir::TurnRight : MoveDir::TurnLeft;
  } else if (in.joyX == 0 && in.joyY == 0) {
    apply(MoveDir::None, in.speedMax, speedMin);  // Speed auch im Stillstand übergeben
    return;
  } else if (fabs(in.joyY) >= fabs(in.joyX)) {
    desiredMove = (in.joyY > 0) ? MoveDir::Forward : MoveDir::Backward;
  } else {
    desiredMove = (in.joyX > 0) ? MoveDir::Right : MoveDir::Left;
  }

  // Geschwindigkeit kommt NUR vom Poti (in.speedMax), nicht proportional
  apply(desiredMove, in.speedMax, speedMin);
}
