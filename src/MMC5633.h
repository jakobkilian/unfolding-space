/*
First try to use MMC5633 by Jakob
 */

#pragma once

#include <stdint.h>

class MMC5633 {
public:
  MMC5633();
  void begin();
  int pollX();
  int pollY();
  int pollZ();

  /*  typedef enum {

      // Note order!
      AFS_2G,
      AFS_16G,
      AFS_4G,
      AFS_8G

    } Ascale_t;


  void calibrate(float *gyroBias, float *accelBias);

  void clearInterrupt(void);

  bool checkNewData(void);

  void readData(float &ax, float &ay, float &az, float &gx, float &gy,
                float &gz);
*/

private:
  // physical address
  static const uint8_t ADDRESS = 0x30;
  // equivalent for wiringpi
  uint8_t i2cAddr;

  // MMC5633 registers

  static const uint8_t Xout0 = 0x00;
  static const uint8_t Xout1 = 0x01;
  static const uint8_t Yout0 = 0x02;
  static const uint8_t Yout1 = 0x03;
  static const uint8_t Zout0 = 0x04;
  static const uint8_t Zout1 = 0x05;
  static const uint8_t Xout2 = 0x06;
  static const uint8_t Yout2 = 0x07;
  static const uint8_t Zout2 = 0x08;
  static const uint8_t Tout = 0x09;
  static const uint8_t TPH0 = 0x0A;
  static const uint8_t TPH1 = 0x0B;
  static const uint8_t TU = 0x0C;
  static const uint8_t Status1 = 0x18;
  static const uint8_t Status0 = 0x19;
  static const uint8_t ODR = 0x1A;
  static const uint8_t CTRL_0 = 0x1B;
  static const uint8_t CTRL_1 = 0x1C;
  static const uint8_t CTRL_2 = 0x1D;
  static const uint8_t ST_X_T = 0x1E;
  static const uint8_t ST_Y_T = 0x1F;
  static const uint8_t ST_Z_T = 20;
  static const uint8_t ST_X = 0x27;
  static const uint8_t ST_Y = 0x28;
  static const uint8_t ST_Z = 0x29;
  static const uint8_t Product_ID = 0x39;

  void seti2cAddr();
  void writeRegister(uint8_t subAddress, uint8_t data);
  uint8_t readRegister(uint8_t subAddress);
  void readRegisters(uint8_t subAddress, uint8_t count, uint8_t *dest);
};
