//----------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------
#include "I2C.hpp"
#include "MotorBoardDefs.hpp"
#include <iostream>

#include "Globals.hpp"
//----------------------------------------------------------------------
// METHODS
//----------------------------------------------------------------------

//________________________________________________
// Just call the wiring pi setup when constructed
I2C::I2C() {
  wiringPiSetup();
  // All I2C communication goes through the two TCA/PCA muxes
  mux[0] = setupDevice(TCA9548A_0_ADDRESS);
  mux[1] = setupDevice(TCA9548A_1_ADDRESS);
  lastMux = 0;
}

//________________________________________________
// In WiringPi every I2C Device has to be initiated to get respective address:
int I2C::setupDevice(int addr) {
  int respectiveAddr = wiringPiI2CSetup(addr);
  if (respectiveAddr < 0) {
    printf("I2C setup of %i failed\n", respectiveAddr);
  }
  return respectiveAddr;
}

//________________________________________________
// select mux and line
int I2C::selectSingleMuxLine(uint8_t muxNo, uint8_t lineNo) {
  // Short explanation how the TCA/PCA works:
  // There is one bit for each of the 8 lines. you can open or close
  // them by writing 0 or 1. Last bit is line 0, second last is line 1 ...
  // 00000001 would close all lines exept from line 0 which is now open
  // 11000000 would open line 7 and line 6 but close the others
  // 00000000 would close all channels. equal to power-up/reset/default
  // selectSingleMuxLine is written to be able to open only one line at a time:
  // It uses bitshifting to shift a "1" by lineNo digits/positions

  int retVal;
  uint8_t regCmd = 1 << lineNo;
  retVal = wiringPiI2CWrite(mux[muxNo], regCmd);
  if (retVal < 0) {
    printf("can't connect to mux %i while setting line %i\n", muxNo, lineNo);
    return -1;
  }
  // if we used the other tca before -> reset
  if (muxNo != lastMux) {
    retVal = wiringPiI2CWrite(mux[lastMux], 0b00000000);
    if (retVal < 0) {
      printf("can't reset mux %i \n", lastMux);
      return -1;
    }
  }
  lastMux = muxNo;
  return 0;
}

//________________________________________________
// set mux by using a binary cmd – here you can open > 1 lines
// here you have to care for the 2nd mux – e.g. reset by using resetMux()
int I2C::manuallySetMux(uint8_t muxNo, uint8_t regCmd) {
  int retVal;
  retVal = wiringPiI2CWrite(mux[muxNo], regCmd);
  if (retVal < 0) {
    printf("can't connect to %i while writing cmd ", muxNo);
    Glob::printBinary(regCmd, true);
    return -1;
  }
  return 0;
}

//________________________________________________
// send a 0 to selcted mux to reset it
int I2C::resetMux(uint8_t muxNo) {
  int retVal;
  retVal = wiringPiI2CWrite(mux[muxNo], 0b00000000);
  if (retVal < 0) {
    printf("can't reset mux %i\n", muxNo);
    return -1;
  }
  return 0;
}

//________________________________________________
// Read data from a register
int I2C::readReg(int addr, unsigned char ucRegAddress) {
  int data;
  if ((data = wiringPiI2CReadReg8(addr, ucRegAddress)) < 0) {
    printf("failed reading 8bit register:  addr = 0x%02x, reg = 0x%02x, "
           "result = %i\n",
           addr, ucRegAddress, data);
    return -1;
  }
  // delayMicroseconds(100); // TODO: really needed? think I read it somewhere.
  return data;
}

//________________________________________________
// Read data from a register
int I2C::readReg16(int addr, unsigned char ucRegAddress) {
  int data;
  if ((data = wiringPiI2CReadReg16(addr, ucRegAddress)) < 0) {
    printf("failed reading 16bit register:  addr = 0x%02x, reg = 0x%02x, "
           "result = %i\n",
           addr, ucRegAddress, data);
    return -1;
  }
  // delayMicroseconds(100); // TODO: really needed? think I read it somewhere.
  return data;
}

//________________________________________________
// Write data to a register
int I2C::writeReg(int addr, unsigned char ucRegAddress, char cValue) {
  int data;
  if ((data = wiringPiI2CWriteReg8(addr, ucRegAddress, cValue)) != 0) {
    printf("failed writing to register:  addr = 0x%02x, reg = 0x%02x, result = "
           "%i\n",
           addr, ucRegAddress, data);
    return -1;
  }
  // delayMicroseconds(100);
  return 0;
}

//________________________________________________
// Write data to a register
int I2C::writeReg16(int addr, unsigned char ucRegAddress, char cValue) {
  int data;
  if ((data = wiringPiI2CWriteReg16(addr, ucRegAddress, cValue)) != 0) {
    printf("failed writing to register:  addr = 0x%02x, reg = 0x%02x, result = "
           "%i\n",
           addr, ucRegAddress, data);
    return -1;
  }
  // delayMicroseconds(100);
  return 0;
}
