#pragma once
#include "CrossPlatformDataBus/CrossPlatformI2C_Core.h"
#include "CrossPlatformDataBus/LSM6DSM.h"
#include "MMC5633.h"
#include <stdint.h>
//****************************************************************
//                          IMU Class
//****************************************************************

class Imu {
public:
  void init();
  void getPosition();
  void printPosition();
  bool onThreshExceeded();
  bool offThreshExceeded();

private:
  int addr;
  static const uint8_t LSM_ADDRESS = 0x6A;
  static const uint8_t LSM_WHOAMI = 0x0F;
  LSM6DSM *lsm6dsm;
  MMC5633 *mmc5633;
  float ax = 0, ay = 0, az = 0, gx = 0, gy = 0, gz = 0;
};