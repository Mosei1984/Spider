#pragma once
#include <Arduino.h>

struct AxisCalib {
  int rawMin = 0;
  int rawMax = 4095;
  int rawCenter = 2048;
  int deadband = 200;
};

struct InputCalibData {
  AxisCalib joyX;
  AxisCalib joyY;
  AxisCalib turn;
  AxisCalib vmax;
  bool valid = false;
};

enum class InputCalibStep {
  IDLE,
  JOY_CENTER,
  JOY_MOVE_ALL,
  TURN_MIN,
  TURN_MAX,
  VMAX_MIN,
  VMAX_MAX,
  DEADBAND,
  SAVING,
  DONE
};

class InputCalib {
public:
  void begin();
  void startCalibration();
  void nextStep();
  void adjustDeadband(int delta);
  void cancelCalibration();
  void update(int rawJoyX, int rawJoyY, int rawTurn, int rawVmax);

  InputCalibStep getStep() const { return step_; }
  const char* getStepText() const;
  const InputCalibData& getData() const { return data_; }
  bool isCalibrated() const { return data_.valid; }
  int getDeadbandPercent() const { return deadbandPercent_; }

  void saveToNVS();
  void loadFromNVS();

private:
  InputCalibStep step_ = InputCalibStep::IDLE;
  InputCalibData data_;
  InputCalibData tempData_;

  int trackMinX_, trackMaxX_;
  int trackMinY_, trackMaxY_;
  int deadbandPercent_ = 5;

  void resetTracking();
  void applyDeadband();
};
