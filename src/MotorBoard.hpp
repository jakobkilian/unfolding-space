#pragma once

//----------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>

#include <array>
#include <ctime>
#include <fstream>
#include <sstream>

//****************************************************************
//                          MotorBoard
//****************************************************************
class MotorBoard {
public:
  void muteAll();
  void setupGlove();
  void sendValuesToGlove(unsigned char values[], int size);
  void runCalib();

private:
  uint8_t initI2CDevice(uint8_t addr);
  int registerWrite(unsigned char, char);
  uint8_t registerRead(unsigned char);
  int drvSelect(uint8_t);
  int setupLRA(bool);
  void resetAll();
  void printStatusToSerial(uint8_t);
  void printSummary();

  uint8_t maxCalibPasses = 2; // max trys for calib before skipping
  uint8_t availableLRAs = 0;  // number of LRAs
  bool calibSuccess[9];       // was calibration successfull?
  int retVal;
  int lastTCA;
  uint8_t order[9] = {0, 2, 1, 3, 4, 5, 8, 7, 6};

  // Should there be a calibration in the beginning? Else: Take standard Values
  bool startupCalib = false;

  // respective I2C Address
  int drv;
  int tca0;
  int tca1;
};