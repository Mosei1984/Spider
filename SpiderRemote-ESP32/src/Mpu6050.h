#pragma once
#include <Arduino.h>
#include <Wire.h>

class Mpu6050 {
public:
  bool begin(TwoWire& wire);
  bool isConnected();
  void readRaw(int16_t& ax, int16_t& ay, int16_t& az, int16_t& gx, int16_t& gy, int16_t& gz);
  void readScaled(float& ax, float& ay, float& az, float& gx, float& gy, float& gz);

private:
  static constexpr uint8_t ADDR = 0x68;
  static constexpr float ACCEL_SCALE = 16384.0f; // ±2g
  static constexpr float GYRO_SCALE = 131.0f;    // ±250°/s

  TwoWire* _wire = nullptr;

  void writeReg(uint8_t reg, uint8_t val);
  uint8_t readReg(uint8_t reg);
};
