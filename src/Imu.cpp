// Jakob 01.07.21:
// Registers of the lsm6dsl are quite extensive and difficult to grasp for me.
// There is a library. But it is also complex. That's why I wrote a short script
// just to check if the chip is available via I2C.
// https://github.com/STMicroelectronics/STMems_Standard_C_drivers/tree/master/lsm6dsl_STdC

//----------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------
#include "Imu.hpp"
#include "Globals.hpp"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h>

//----------------------------------------------------------------------
// METHODS
//----------------------------------------------------------------------
void Imu::init() {
  {
    std::lock_guard<std::mutex> locki2c(Glob::i2cMux);
    Glob::i2c.selectSingleMuxLine(1, 7);
    Glob::i2c.appendMuxMask(1, 1 << 7);
  }
  /* Specify sensor parameters (sample rate is twice the bandwidth)
  * choices are:
  AFS_2G, AFS_4G, AFS_8G, AFS_16G
  GFS_245DPS, GFS_500DPS, GFS_1000DPS, GFS_2000DPS
  AODR_12_5Hz, AODR_26Hz, AODR_52Hz, AODR_104Hz, AODR_208Hz, AODR_416Hz,
  AODR_833Hz, AODR_1660Hz, AODR_3330Hz, AODR_6660Hz GODR_12_5Hz, GODR_26Hz,
  GODR_52Hz, GODR_104Hz, GODR_208Hz, GODR_416Hz, GODR_833Hz, GODR_1660Hz,
  GODR_3330Hz, GODR_6660Hz
  */
  static LSM6DSM::Ascale_t Ascale = LSM6DSM::AFS_2G;
  static LSM6DSM::Gscale_t Gscale = LSM6DSM::GFS_245DPS;
  static LSM6DSM::Rate_t AODR = LSM6DSM::ODR_833Hz;
  static LSM6DSM::Rate_t GODR = LSM6DSM::ODR_833Hz;
  // Biases computed by Kris
  static float ACCEL_BIAS[3] = {-0.01308, -0.00493, 0.03083};
  static float GYRO_BIAS[3] = {0.71, -2.69, 0.78};
  lsm6dsm = new LSM6DSM(Ascale, Gscale, AODR, GODR, ACCEL_BIAS, GYRO_BIAS);

  switch (lsm6dsm->begin()) {
  case LSM6DSM::ERROR_ID:
    printf("chip ID");
    break;
  case LSM6DSM::ERROR_SELFTEST:
    printf("self-test");
    break;
  default:
    break;
  }

  // MMC
  mmc5633 = new MMC5633();
  mmc5633->begin();
}

void Imu::getPosition() {
  if (lsm6dsm->checkNewData()) {
    ax = 0, ay = 0, az = 0, gx = 0, gy = 0, gz = 0;
    lsm6dsm->readData(ax, ay, az, gx, gy, gz);
    ax *= 1000, ay *= 1000, az *= 1000, gx *= 10, gy *= 10, gz *= 10;
  }
}

bool Imu::offThreshExceeded() {
  bool exceeded = (ax > -500 || abs(ay) > 700) ? true : false;
  return exceeded;
}

bool Imu::onThreshExceeded() {
  bool exceeded = (ax < -600 && abs(ay) < 600) ? true : false;
  return exceeded;
}

void Imu::printPosition() {
  printf("ax = %5.0f \t", ax);
  printf("ay = %5.0f \t", ay);
  printf("az = %5.0f \t\t", az);
  printf("gx = %5.0f \t", gx);
  printf("gy = %5.0f \t", gy);
  printf("gz = %5.0f", gz);
  printf("\n");
}
