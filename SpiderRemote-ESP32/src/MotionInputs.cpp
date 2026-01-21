#include "MotionInputs.h"
#include "Config.h"
#include <math.h>

static constexpr uint8_t MPU_ADDR = 0x68;
static constexpr float ACCEL_SCALE = 16384.0f;  // ±2g
static constexpr float GYRO_SCALE  = 131.0f;    // ±250°/s
static constexpr float DEG2RAD = 0.017453292f;
static constexpr float RAD2DEG = 57.29577951f;

bool MotionInputs::begin(TwoWire& wire) {
  _wire = &wire;
  _available = initMpu();
  if (_available) {
    lastUs = micros();
  }
  return _available;
}

bool MotionInputs::isAvailable() const {
  return _available;
}

bool MotionInputs::isCalibrated() const {
  return _calibrated;
}

bool MotionInputs::initMpu() {
  _wire->beginTransmission(MPU_ADDR);
  _wire->write(0x6B);  // PWR_MGMT_1
  _wire->write(0x00);  // wake up
  if (_wire->endTransmission(true) != 0) return false;
  delay(10);

  _wire->beginTransmission(MPU_ADDR);
  _wire->write(0x1B);  // GYRO_CONFIG
  _wire->write(0x00);  // ±250°/s
  _wire->endTransmission(true);

  _wire->beginTransmission(MPU_ADDR);
  _wire->write(0x1C);  // ACCEL_CONFIG
  _wire->write(0x00);  // ±2g
  _wire->endTransmission(true);

  _wire->beginTransmission(MPU_ADDR);
  _wire->write(0x1A);  // CONFIG (DLPF)
  _wire->write(0x03);  // ~44Hz bandwidth
  _wire->endTransmission(true);

  return true;
}

bool MotionInputs::readRaw(float& ax, float& ay, float& az, float& gx, float& gy, float& gz) {
  _wire->beginTransmission(MPU_ADDR);
  _wire->write(0x3B);  // ACCEL_XOUT_H
  if (_wire->endTransmission(false) != 0) return false;

  if (_wire->requestFrom((int)MPU_ADDR, 14) != 14) return false;

  int16_t axRaw = (_wire->read() << 8) | _wire->read();
  int16_t ayRaw = (_wire->read() << 8) | _wire->read();
  int16_t azRaw = (_wire->read() << 8) | _wire->read();
  _wire->read(); _wire->read();  // skip temp
  int16_t gxRaw = (_wire->read() << 8) | _wire->read();
  int16_t gyRaw = (_wire->read() << 8) | _wire->read();
  int16_t gzRaw = (_wire->read() << 8) | _wire->read();

  ax = axRaw / ACCEL_SCALE;
  ay = ayRaw / ACCEL_SCALE;
  az = azRaw / ACCEL_SCALE;
  gx = gxRaw / GYRO_SCALE;
  gy = gyRaw / GYRO_SCALE;
  gz = gzRaw / GYRO_SCALE;

  return true;
}

void MotionInputs::calibrate() {
  startCalibration();
  while (calibState == CalibState::WAITING || calibState == CalibState::RUNNING) {
    updateCalibration();
    delay(5);
  }
}

void MotionInputs::startCalibration() {
  if (!_available) {
    calibState = CalibState::FAILED;
    return;
  }
  calibState = CalibState::WAITING;
  calibStartMs = millis();
  calibSumPitch = 0;
  calibSumRoll = 0;
  calibSumGz = 0;
  calibCount = 0;
}

void MotionInputs::updateCalibration() {
  if (calibState == CalibState::WAITING) {
    calibState = CalibState::RUNNING;
    calibStartMs = millis();
  }

  if (calibState != CalibState::RUNNING) return;

  uint32_t elapsed = millis() - calibStartMs;
  if (elapsed >= CALIB_DURATION_MS) {
    if (calibCount > 0) {
      pitch0 = calibSumPitch / calibCount;
      roll0  = calibSumRoll  / calibCount;
      gzBias = calibSumGz    / calibCount;
      pitch  = 0;
      roll   = 0;
      lastUs = micros();
      _calibrated = true;
      calibState = CalibState::DONE;
    } else {
      calibState = CalibState::FAILED;
    }
    return;
  }

  float ax, ay, az, gx, gy, gz;
  if (readRaw(ax, ay, az, gx, gy, gz)) {
    float accelPitch = atan2(-ax, sqrtf(ay*ay + az*az)) * RAD2DEG;
    float accelRoll  = atan2(ay, az) * RAD2DEG;
    calibSumPitch += accelPitch;
    calibSumRoll  += accelRoll;
    calibSumGz    += gz;
    calibCount++;
  }
}

void MotionInputs::resetCalibState() {
  calibState = CalibState::IDLE;
}

CalibState MotionInputs::getCalibState() const {
  return calibState;
}

int MotionInputs::getCalibProgress() const {
  if (calibState != CalibState::RUNNING) return 0;
  uint32_t elapsed = millis() - calibStartMs;
  return constrain((int)(elapsed * 100 / CALIB_DURATION_MS), 0, 100);
}

InputState MotionInputs::read(int speedMax) {
  InputState state = {speedMax, 0, 0, 0};
  if (!_available || !_calibrated) return state;

  float ax, ay, az, gx, gy, gz;
  if (!readRaw(ax, ay, az, gx, gy, gz)) return state;

  uint32_t nowUs = micros();
  float dt = (nowUs - lastUs) * 1e-6f;
  lastUs = nowUs;
  if (dt <= 0 || dt > 0.5f) dt = 0.01f;

  float accelPitch = atan2(-ax, sqrtf(ay*ay + az*az)) * RAD2DEG - pitch0;
  float accelRoll  = atan2(ay, az) * RAD2DEG - roll0;

  float gyroPitchRate = gy;  // pitch rotation around Y
  float gyroRollRate  = gx;  // roll rotation around X

  pitch = MOTION_ALPHA * (pitch + gyroPitchRate * dt) + (1.0f - MOTION_ALPHA) * accelPitch;
  roll  = MOTION_ALPHA * (roll  + gyroRollRate  * dt) + (1.0f - MOTION_ALPHA) * accelRoll;

  float normPitch = clampf(pitch / TILT_MAX_DEG, -1.0f, 1.0f);
  float normRoll  = clampf(roll  / TILT_MAX_DEG, -1.0f, 1.0f);

  float gzCorrected = gz - gzBias;
  float normTurn = clampf(-gzCorrected / TURN_MAX_DPS, -1.0f, 1.0f);  // Invertiert

  state.joyY = applyDeadzone(normPitch, DEAD_TILT);
  state.joyX = applyDeadzone(normRoll,  DEAD_TILT);
  state.turn = applyDeadzone(normTurn,  DEAD_TURN_MOTION);

  return state;
}

float MotionInputs::applyDeadzone(float val, float dead) const {
  if (fabsf(val) < dead) return 0.0f;
  float sign = (val > 0) ? 1.0f : -1.0f;
  return sign * (fabsf(val) - dead) / (1.0f - dead);
}

float MotionInputs::clampf(float val, float lo, float hi) const {
  if (val < lo) return lo;
  if (val > hi) return hi;
  return val;
}
