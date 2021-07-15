#pragma once

#include <stdint.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>

//****************************************************************
//                          I2C Class
//****************************************************************
// Class that handels all the i2c communication to the various devices

class I2C {
public:
  I2C();
  int setupDevice(int addr);
  int selectSingleMuxLine(uint8_t mux, uint8_t line);
  int manuallySetMux(uint8_t muxNo, uint8_t regCmd);
  int resetMux(uint8_t muxNo);
  int readReg(int addr, unsigned char ucRegAddress);
  int readReg16(int addr, unsigned char ucRegAddress);
  int writeReg(int addr, unsigned char ucRegAddress, char cValue);
    int writeReg16(int addr, unsigned char ucRegAddress, char cValue);

private:
  int mux[2];
  int lastMux;
  int printBinary(uint8_t, bool);
};