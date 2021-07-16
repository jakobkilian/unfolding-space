/*
   LSM6DSM.cpp: Implementation of LSM6DSM class

   Copyright (C) 2018 Simon D. Levy

   Adapted from https://github.com/kriswiner/LIS2MDL_LPS22HB

   This file is part of LSM6DSM.

   LSM6DSM is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   LSM6DSM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with LSM6DSM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "LSM6DSM.h"

#include "CrossPlatformI2C_Core.h"

#include <math.h>

LSM6DSM::LSM6DSM(Ascale_t ascale, Gscale_t gscale, Rate_t aodr, Rate_t godr,
                 float accelBias[3], float gyroBias[3]) {
  _ascale = ascale;
  _gscale = gscale;
  _aodr = aodr;
  _godr = godr;

  float areses[4] = {2, 16, 4, 8};
  _ares = areses[ascale] / 32768.f;

  float greses[4] = {245, 500, 1000, 2000};
  _gres = greses[gscale] / 32768.f;

  for (uint8_t k = 0; k < 3; ++k) {
    _accelBias[k] = accelBias[k];
    _gyroBias[k] = gyroBias[k];
  }
}

LSM6DSM::LSM6DSM(Ascale_t ascale, Gscale_t gscale, Rate_t aodr, Rate_t godr) {
  float accelBias[3] = {0, 0, 0};
  float gyroBias[3] = {0, 0, 0};
  LSM6DSM(ascale, gscale, aodr, godr, accelBias, gyroBias);
}

LSM6DSM::Error_t LSM6DSM::begin(uint8_t bus) {
  _i2c = cpi2c_open(ADDRESS, bus); // Support for wiringPi, I2CDEV

  if (readRegister(WHO_AM_I) != 0x6A) {
    return ERROR_ID;
  }

  // reset device
  uint8_t temp = readRegister(CTRL3_C);
  writeRegister(CTRL3_C, temp | 0x01); // Set bit 0 to 1 to reset LSM6DSM
  delay(100);                          // Wait for all registers to reset
  writeRegister(CTRL1_XL, _aodr << 4 | _ascale << 2);

  writeRegister(CTRL2_G, _godr << 4 | _gscale << 2);

  temp = readRegister(CTRL3_C);

  // enable block update (bit 6 = 1), auto-increment registers (bit 2 = 1)
  writeRegister(CTRL3_C, temp | 0x40 | 0x04);
  // by default, interrupts active HIGH, push pull, little endian data
  // (can be changed by writing to bits 5, 4, and 1, resp to above register)

  // enable accel LP2 (bit 7 = 1), set LP2 tp ODR/9 (bit 6 = 1), enable
  // input_composite (bit 3) for low noise
  writeRegister(CTRL8_XL, 0x80 | 0x40 | 0x08);

  // interrupt handling
  writeRegister(DRDY_PULSE_CFG, 0x80); // latch interrupt until data read
  writeRegister(INT1_CTRL, 0x03);      // enable  data ready interrupts on INT1
  writeRegister(INT2_CTRL,
                0x40); // enable significant motion interrupts on INT2

  return selfTest() ? ERROR_NONE : ERROR_SELFTEST;
}

void LSM6DSM::readData(float &ax, float &ay, float &az, float &gx, float &gy,
                       float &gz) {
  int16_t data[7];

  readData(data);

  // Calculate the accleration value into actual g's
  ax = (float)data[4] * _ares -
       _accelBias[0]; // get actual g value, this depends on scale being set
  ay = (float)data[5] * _ares - _accelBias[1];
  az = (float)data[6] * _ares - _accelBias[2];

  // Calculate the gyro value into actual degrees per second
  gx = (float)data[1] * _gres -
       _gyroBias[0]; // get actual gyro value, this depends on scale being set
  gy = (float)data[2] * _gres - _gyroBias[1];
  gz = (float)data[3] * _gres - _gyroBias[2];
}

bool LSM6DSM::checkNewData(void) {
  // use the gyro bit to check new data
  return (bool)(readRegister(STATUS_REG) & 0x02);
}

bool LSM6DSM::selfTest() {
  int16_t temp[7] = {0, 0, 0, 0, 0, 0, 0};
  int16_t accelPTest[3] = {0, 0, 0};
  int16_t accelNTest[3] = {0, 0, 0};
  int16_t gyroPTest[3] = {0, 0, 0};
  int16_t gyroNTest[3] = {0, 0, 0};
  int16_t accelNom[3] = {0, 0, 0};
  int16_t gyroNom[3] = {0, 0, 0};

  readData(temp);
  accelNom[0] = temp[4];
  accelNom[1] = temp[5];
  accelNom[2] = temp[6];
  gyroNom[0] = temp[1];
  gyroNom[1] = temp[2];
  gyroNom[2] = temp[3];

  writeRegister(CTRL5_C, 0x01); // positive accel self test
  delay(100);                   // let accel respond
  readData(temp);
  accelPTest[0] = temp[4];
  accelPTest[1] = temp[5];
  accelPTest[2] = temp[6];

  writeRegister(CTRL5_C, 0x03); // negative accel self test
  delay(100);                   // let accel respond
  readData(temp);
  accelNTest[0] = temp[4];
  accelNTest[1] = temp[5];
  accelNTest[2] = temp[6];

  writeRegister(CTRL5_C, 0x04); // positive gyro self test
  delay(100);                   // let gyro respond
  readData(temp);
  gyroPTest[0] = temp[1];
  gyroPTest[1] = temp[2];
  gyroPTest[2] = temp[3];

  writeRegister(CTRL5_C, 0x0C); // negative gyro self test
  delay(100);                   // let gyro respond
  readData(temp);
  gyroNTest[0] = temp[1];
  gyroNTest[1] = temp[2];
  gyroNTest[2] = temp[3];

  writeRegister(CTRL5_C, 0x00); // normal mode
  delay(100);                   // let accel and gyro respond

  return inBounds(accelPTest, accelNTest, accelNom, _ares, ACCEL_MIN,
                  ACCEL_MAX) &&
         inBounds(gyroPTest, gyroNTest, gyroNom, _gres, GYRO_MIN, GYRO_MAX);
}

bool LSM6DSM::inBounds(int16_t ptest[3], int16_t ntest[3], int16_t nom[3],
                       float res, float minval, float maxval) {
  for (uint8_t k = 0; k < 3; ++k) {

    if (outOfBounds((ptest[k] - nom[k]) * res, minval, maxval) ||
        outOfBounds((ntest[k] - nom[k]) * res, minval, maxval)) {

      return false;
    }
  }

  return true;
}

bool LSM6DSM::outOfBounds(float val, float minval, float maxval) {
  val = fabs(val);
  return val < minval || val > maxval;
}

void LSM6DSM::calibrate(float *gyroBias, float *accelBias) {
  int16_t temp[7] = {0, 0, 0, 0, 0, 0, 0};
  int32_t sum[7] = {0, 0, 0, 0, 0, 0, 0};

  for (int ii = 0; ii < 128; ii++) {
    readData(temp);
    sum[1] += temp[1];
    sum[2] += temp[2];
    sum[3] += temp[3];
    sum[4] += temp[4];
    sum[5] += temp[5];
    sum[6] += temp[6];
    delay(50);
  }

  gyroBias[0] = sum[1] * _gres / 128.0f;
  gyroBias[1] = sum[2] * _gres / 128.0f;
  gyroBias[2] = sum[3] * _gres / 128.0f;
  accelBias[0] = sum[4] * _ares / 128.0f;
  accelBias[1] = sum[5] * _ares / 128.0f;
  accelBias[2] = sum[6] * _ares / 128.0f;

  if (accelBias[0] > 0.8f) {
    accelBias[0] -= 1.0f;
  } // Remove gravity from the x-axis accelerometer bias calculation
  if (accelBias[0] < -0.8f) {
    accelBias[0] += 1.0f;
  } // Remove gravity from the x-axis accelerometer bias calculation
  if (accelBias[1] > 0.8f) {
    accelBias[1] -= 1.0f;
  } // Remove gravity from the y-axis accelerometer bias calculation
  if (accelBias[1] < -0.8f) {
    accelBias[1] += 1.0f;
  } // Remove gravity from the y-axis accelerometer bias calculation
  if (accelBias[2] > 0.8f) {
    accelBias[2] -= 1.0f;
  } // Remove gravity from the z-axis accelerometer bias calculation
  if (accelBias[2] < -0.8f) {
    accelBias[2] += 1.0f;
  } // Remove gravity from the z-axis accelerometer bias calculation

  for (uint8_t k = 0; k < 3; ++k) {
    _accelBias[k] = accelBias[k];
    _gyroBias[k] = gyroBias[k];
  }
}

void LSM6DSM::clearInterrupt(void) {
  int16_t data[7];
  readData(data);
}

void LSM6DSM::readData(int16_t destination[7]) {
  uint8_t rawData[14]; // x/y/z accel register data stored here
  readRegisters(OUT_TEMP_L, 14,
                &rawData[0]); // Read the 14 raw data registers into data array
  destination[0] =
      ((int16_t)rawData[1] << 8) |
      rawData[0]; // Turn the MSB and LSB into a signed 16-bit value
  destination[1] = ((int16_t)rawData[3] << 8) | rawData[2];
  destination[2] = ((int16_t)rawData[5] << 8) | rawData[4];
  destination[3] = ((int16_t)rawData[7] << 8) | rawData[6];
  destination[4] = ((int16_t)rawData[9] << 8) | rawData[8];
  destination[5] = ((int16_t)rawData[11] << 8) | rawData[10];
  destination[6] = ((int16_t)rawData[13] << 8) | rawData[12];
}

uint8_t LSM6DSM::readRegister(uint8_t subAddress) {
  uint8_t temp;
  readRegisters(subAddress, 1, &temp);
  return temp;
}

void LSM6DSM::writeRegister(uint8_t subAddress, uint8_t data) {
  cpi2c_writeRegister(_i2c, subAddress, data);
}

void LSM6DSM::readRegisters(uint8_t subAddress, uint8_t count, uint8_t *dest) {
  cpi2c_readRegisters(_i2c, subAddress, count, dest);
}
