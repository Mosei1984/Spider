#include "Mpu6050.h"

static constexpr uint8_t REG_PWR_MGMT_1   = 0x6B;
static constexpr uint8_t REG_GYRO_CONFIG  = 0x1B;
static constexpr uint8_t REG_ACCEL_CONFIG = 0x1C;
static constexpr uint8_t REG_WHO_AM_I     = 0x75;
static constexpr uint8_t REG_ACCEL_XOUT_H = 0x3B;

bool Mpu6050::begin(TwoWire& wire) {
  _wire = &wire;

  if (!isConnected()) return false;

  writeReg(REG_PWR_MGMT_1, 0x00);   // wake up
  writeReg(REG_GYRO_CONFIG, 0x00);  // ±250°/s
  writeReg(REG_ACCEL_CONFIG, 0x00); // ±2g

  return true;
}

bool Mpu6050::isConnected() {
  if (!_wire) return false;
  uint8_t id = readReg(REG_WHO_AM_I);
  return (id == 0x68 || id == 0x72);
}

void Mpu6050::readRaw(int16_t& ax, int16_t& ay, int16_t& az,
                      int16_t& gx, int16_t& gy, int16_t& gz) {
  _wire->beginTransmission(ADDR);
  _wire->write(REG_ACCEL_XOUT_H);
  _wire->endTransmission(false);
  _wire->requestFrom(ADDR, (uint8_t)14);

  ax = (_wire->read() << 8) | _wire->read();
  ay = (_wire->read() << 8) | _wire->read();
  az = (_wire->read() << 8) | _wire->read();
  _wire->read(); _wire->read(); // skip temperature
  gx = (_wire->read() << 8) | _wire->read();
  gy = (_wire->read() << 8) | _wire->read();
  gz = (_wire->read() << 8) | _wire->read();
}

void Mpu6050::readScaled(float& ax, float& ay, float& az,
                         float& gx, float& gy, float& gz) {
  int16_t rax, ray, raz, rgx, rgy, rgz;
  readRaw(rax, ray, raz, rgx, rgy, rgz);

  ax = rax / ACCEL_SCALE;
  ay = ray / ACCEL_SCALE;
  az = raz / ACCEL_SCALE;
  gx = rgx / GYRO_SCALE;
  gy = rgy / GYRO_SCALE;
  gz = rgz / GYRO_SCALE;
}

void Mpu6050::writeReg(uint8_t reg, uint8_t val) {
  _wire->beginTransmission(ADDR);
  _wire->write(reg);
  _wire->write(val);
  _wire->endTransmission();
}

uint8_t Mpu6050::readReg(uint8_t reg) {
  _wire->beginTransmission(ADDR);
  _wire->write(reg);
  _wire->endTransmission(false);
  _wire->requestFrom(ADDR, (uint8_t)1);
  return _wire->read();
}
