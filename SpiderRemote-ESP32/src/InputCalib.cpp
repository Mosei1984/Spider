#include "InputCalib.h"
#include <Preferences.h>

static Preferences prefs;
static const char* NVS_NAMESPACE = "inputcalib";

void InputCalib::begin() {
  loadFromNVS();
}

void InputCalib::loadFromNVS() {
  prefs.begin(NVS_NAMESPACE, true);
  data_.valid = prefs.getBool("valid", false);
  if (data_.valid) {
    data_.joyX.rawMin    = prefs.getInt("joyXmin", 0);
    data_.joyX.rawMax    = prefs.getInt("joyXmax", 4095);
    data_.joyX.rawCenter = prefs.getInt("joyXctr", 2048);
    data_.joyX.deadband  = prefs.getInt("joyXdead", 200);

    data_.joyY.rawMin    = prefs.getInt("joyYmin", 0);
    data_.joyY.rawMax    = prefs.getInt("joyYmax", 4095);
    data_.joyY.rawCenter = prefs.getInt("joyYctr", 2048);
    data_.joyY.deadband  = prefs.getInt("joyYdead", 200);

    data_.turn.rawMin    = prefs.getInt("turnMin", 0);
    data_.turn.rawMax    = prefs.getInt("turnMax", 4095);
    data_.turn.rawCenter = prefs.getInt("turnCtr", 2048);
    data_.turn.deadband  = prefs.getInt("turnDead", 200);

    data_.vmax.rawMin    = prefs.getInt("vmaxMin", 0);
    data_.vmax.rawMax    = prefs.getInt("vmaxMax", 4095);
    data_.vmax.rawCenter = prefs.getInt("vmaxCtr", 2048);
    data_.vmax.deadband  = prefs.getInt("vmaxDead", 0);
  }
  prefs.end();
}

void InputCalib::saveToNVS() {
  prefs.begin(NVS_NAMESPACE, false);
  prefs.putBool("valid", data_.valid);

  prefs.putInt("joyXmin", data_.joyX.rawMin);
  prefs.putInt("joyXmax", data_.joyX.rawMax);
  prefs.putInt("joyXctr", data_.joyX.rawCenter);
  prefs.putInt("joyXdead", data_.joyX.deadband);

  prefs.putInt("joyYmin", data_.joyY.rawMin);
  prefs.putInt("joyYmax", data_.joyY.rawMax);
  prefs.putInt("joyYctr", data_.joyY.rawCenter);
  prefs.putInt("joyYdead", data_.joyY.deadband);

  prefs.putInt("turnMin", data_.turn.rawMin);
  prefs.putInt("turnMax", data_.turn.rawMax);
  prefs.putInt("turnCtr", data_.turn.rawCenter);
  prefs.putInt("turnDead", data_.turn.deadband);

  prefs.putInt("vmaxMin", data_.vmax.rawMin);
  prefs.putInt("vmaxMax", data_.vmax.rawMax);
  prefs.putInt("vmaxCtr", data_.vmax.rawCenter);
  prefs.putInt("vmaxDead", data_.vmax.deadband);

  prefs.end();
}

void InputCalib::startCalibration() {
  tempData_ = InputCalibData();
  deadbandPercent_ = 5;
  step_ = InputCalibStep::JOY_CENTER;
}

void InputCalib::cancelCalibration() {
  step_ = InputCalibStep::IDLE;
}

void InputCalib::resetTracking() {
  trackMinX_ = 4095;
  trackMaxX_ = 0;
  trackMinY_ = 4095;
  trackMaxY_ = 0;
}

void InputCalib::applyDeadband() {
  int rangeX = tempData_.joyX.rawMax - tempData_.joyX.rawMin;
  int rangeY = tempData_.joyY.rawMax - tempData_.joyY.rawMin;
  int rangeT = tempData_.turn.rawMax - tempData_.turn.rawMin;

  tempData_.joyX.deadband = (rangeX * deadbandPercent_) / 100;
  tempData_.joyY.deadband = (rangeY * deadbandPercent_) / 100;
  tempData_.turn.deadband = (rangeT * deadbandPercent_) / 100;
  tempData_.vmax.deadband = 0;
}

void InputCalib::nextStep() {
  switch (step_) {
    case InputCalibStep::IDLE:
      break;

    case InputCalibStep::JOY_CENTER:
      resetTracking();
      step_ = InputCalibStep::JOY_MOVE_ALL;
      break;

    case InputCalibStep::JOY_MOVE_ALL:
      tempData_.joyX.rawMin = trackMinX_;
      tempData_.joyX.rawMax = trackMaxX_;
      tempData_.joyY.rawMin = trackMinY_;
      tempData_.joyY.rawMax = trackMaxY_;
      step_ = InputCalibStep::TURN_MIN;
      break;

    case InputCalibStep::TURN_MIN:
      step_ = InputCalibStep::TURN_MAX;
      break;

    case InputCalibStep::TURN_MAX:
      step_ = InputCalibStep::VMAX_MIN;
      break;

    case InputCalibStep::VMAX_MIN:
      step_ = InputCalibStep::VMAX_MAX;
      break;

    case InputCalibStep::VMAX_MAX:
      applyDeadband();
      step_ = InputCalibStep::DEADBAND;
      break;

    case InputCalibStep::DEADBAND:
      step_ = InputCalibStep::SAVING;
      tempData_.valid = true;
      data_ = tempData_;
      saveToNVS();
      step_ = InputCalibStep::DONE;
      break;

    case InputCalibStep::SAVING:
    case InputCalibStep::DONE:
      step_ = InputCalibStep::IDLE;
      break;
  }
}

void InputCalib::adjustDeadband(int delta) {
  if (step_ != InputCalibStep::DEADBAND) return;
  deadbandPercent_ += delta;
  if (deadbandPercent_ < 0) deadbandPercent_ = 0;
  if (deadbandPercent_ > 20) deadbandPercent_ = 20;
  applyDeadband();
}

void InputCalib::update(int rawJoyX, int rawJoyY, int rawTurn, int rawVmax) {
  switch (step_) {
    case InputCalibStep::JOY_CENTER:
      tempData_.joyX.rawCenter = rawJoyX;
      tempData_.joyY.rawCenter = rawJoyY;
      break;

    case InputCalibStep::JOY_MOVE_ALL:
      if (rawJoyX < trackMinX_) trackMinX_ = rawJoyX;
      if (rawJoyX > trackMaxX_) trackMaxX_ = rawJoyX;
      if (rawJoyY < trackMinY_) trackMinY_ = rawJoyY;
      if (rawJoyY > trackMaxY_) trackMaxY_ = rawJoyY;
      break;

    case InputCalibStep::TURN_MIN:
      tempData_.turn.rawMin = rawTurn;
      break;

    case InputCalibStep::TURN_MAX:
      tempData_.turn.rawMax = rawTurn;
      tempData_.turn.rawCenter = (tempData_.turn.rawMin + tempData_.turn.rawMax) / 2;
      break;

    case InputCalibStep::VMAX_MIN:
      tempData_.vmax.rawMin = rawVmax;
      break;

    case InputCalibStep::VMAX_MAX:
      tempData_.vmax.rawMax = rawVmax;
      tempData_.vmax.rawCenter = (tempData_.vmax.rawMin + tempData_.vmax.rawMax) / 2;
      break;

    default:
      break;
  }
}

const char* InputCalib::getStepText() const {
  switch (step_) {
    case InputCalibStep::IDLE:         return "Kalibrierung bereit";
    case InputCalibStep::JOY_CENTER:   return "Joystick loslassen\n-> MID druecken";
    case InputCalibStep::JOY_MOVE_ALL: return "Joystick in alle\nEcken -> MID";
    case InputCalibStep::TURN_MIN:     return "Drehpoti ganz links\n-> MID druecken";
    case InputCalibStep::TURN_MAX:     return "Drehpoti ganz rechts\n-> MID druecken";
    case InputCalibStep::VMAX_MIN:     return "Speed-Poti ganz links\n-> MID druecken";
    case InputCalibStep::VMAX_MAX:     return "Speed-Poti ganz rechts\n-> MID druecken";
    case InputCalibStep::DEADBAND:     return "Deadband L/R anpassen\n-> MID speichern";
    case InputCalibStep::SAVING:       return "Speichere...";
    case InputCalibStep::DONE:         return "Kalibrierung fertig!";
  }
  return "";
}
