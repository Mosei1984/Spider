#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "Inputs.h"

enum class CalibState { IDLE, WAITING, RUNNING, DONE, FAILED };

class MotionInputs {
public:
  bool begin(TwoWire& wire);
  bool isAvailable() const;
  void calibrate();
  bool isCalibrated() const;
  InputState read(int speedMax);

  void startCalibration();
  void updateCalibration();
  void resetCalibState();
  CalibState getCalibState() const;
  int getCalibProgress() const;

private:
  TwoWire* _wire = nullptr;
  bool _available = false;
  bool _calibrated = false;

  float pitch0 = 0.0f;
  float roll0  = 0.0f;
  float gzBias = 0.0f;

  float pitch = 0.0f;
  float roll  = 0.0f;
  uint32_t lastUs = 0;

  CalibState calibState = CalibState::IDLE;
  uint32_t calibStartMs = 0;
  float calibSumPitch = 0, calibSumRoll = 0, calibSumGz = 0;
  int calibCount = 0;
  static constexpr uint32_t CALIB_DURATION_MS = 2000;

  bool initMpu();
  bool readRaw(float& ax, float& ay, float& az, float& gx, float& gy, float& gz);
  float applyDeadzone(float val, float dead) const;
  float clampf(float val, float lo, float hi) const;
};
