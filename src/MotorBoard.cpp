//----------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------
#include "MotorBoard.hpp"

#include <iostream>

#include "Camera.hpp"
#include "Globals.hpp"
#include "MotorBoardDefs.hpp"
#include "TimeLogger.hpp"

//----------------------------------------------------------------------
// METHODS
//----------------------------------------------------------------------

// initially set up all TCA9548A, DRV2605 and the actuators
void MotorBoard::setupGlove() {
  drv = Glob::i2c.setupDevice(DRV2605_ADDRESS);
  // resetAll(); //to stop ongoing vibrations or faulty settings
  // Write settings to all drivers and start simultaneous auto calibration
  for (int u = 0; u < 9; u++) {
    drvSelect(u); // select the driver to write to
    while (setupLRA(startupCalib) != 0) {
      // printf("writing settings to no %i failed \n", u); //Send all register
      // settings and "GO" bit to start auto calibration
    }
  }
  // the following calibration process can be skipped (startupCalib=false).
  // The standard values do their job well enough
  if (startupCalib) {
    runCalib();
  }
}

void MotorBoard::sendValuesToGlove(unsigned char inValues[], int size) {
  // WRITE VALUES TO GLOVE
  unsigned char values[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
  Glob::logger.motorSendLog.reset();
  Glob::logger.motorSendLog.store("startSendGlove");
  {
    for (int i = 0; i < size; i++) {
      // stronger vibrations on left motors
      if (i == 0 || i == 3 || i == 6) {
        values[i] = static_cast<int>(inValues[i] * 0.8);
      } else {
        values[i] = static_cast<int>(inValues[i] * 0.6);
      }
    }
  }
  Glob::logger.motorSendLog.store("copy");
  // Write Values to the registers of the motor drivers (drv..)
  // All drv have same addr. -> two i2c multiplexer (tca) are needed.
  if (!Glob::modes.a_muted && !Glob::royalStats.a_isCalibRunning) {
    // For speed's sake start with drvs that are on the first tca
    for (int i = 0; i < size; ++i) {
      if (order[i] <= 4) {
        drvSelect(order[i]); // route the value to the right TCA and DRV
        protectedWrite(drv, RTP_INPUT, altCurve[values[i]]);
      }
    }
    Glob::logger.motorSendLog.store("TCA1");
    // Now all drv on the 2nd tca together
    for (int i = 0; i < size; ++i) {
      if (order[i] > 4) {
        drvSelect(order[i]); // route the value to the right TCA and DRV
        protectedWrite(drv, RTP_INPUT, altCurve[values[i]]);
      }
    }
  }
  Glob::logger.motorSendLog.store("TCA2");
  Glob::logger.motorSendLog.store("end");
  Glob::logger.motorSendLog.printAll("SEND VALUES TO MOTORS", "us", "ms");
  Glob::logger.motorSendLog.udpTimeSpan("processing", "us", "startProcess",
                                        "endProcess");
  Glob::logger.motorSendLog.udpTimeSpan("onNewData", "us", "start",
                                        "notifyProcessing");
  Glob::logger.motorSendLog.udpTimeSpan("gloveSending", "us", "startSendGlove",
                                        "end");
  Glob::logger.motorSendLog.udpTimeSpan("wholeCycle", "us", "start", "end");
  Glob::logger.motorSendLog.reset();
  Glob::logger.pauseLog.reset();
  Glob::logger.pauseLog.store("startPause");
}

//________________________________________________
// Route DRV 0-4 to first TCA multiplexer and 5-8 to the second TCA
int MotorBoard::drvSelect(uint8_t drvNo) {
  std::lock_guard<std::mutex> locki2c(Glob::i2cMux);
  if (drvNo <= 4) {
    Glob::i2c.selectSingleMuxLine(0, drvNo);
  } else if (drvNo > 4) {
    // substract 5 as the 6th drv is on position 0 again
    Glob::i2c.selectSingleMuxLine(1, drvNo - 5);
  }
  return 0;
}

//________________________________________________
// Set the settings of the DRVs by writing to their registers
int MotorBoard::setupLRA(bool calib) {
  if (protectedWrite(drv, MODE, 0x07) != 0)
    return -1;
  if (protectedWrite(drv, FB_CON, 0b11000000) != 0)
    return -1;
  if (protectedWrite(drv, RATED_VOLTAGE, 0b01101000) != 0)
    return -1;
  if (protectedWrite(drv, OD_CLAMP, 0b10001000) != 0)
    return -1;
  if (protectedWrite(drv, CONTRL1, 0b10011000) != 0)
    return -1;
  if (protectedWrite(drv, CONTRL2, 0b01000101) != 0)
    return -1;
  if (protectedWrite(drv, CONTRL3, 0b00001000) != 0)
    return -1;
  if (protectedWrite(drv, CONTRL4, 0b00000000) != 0)
    return -1;
  if (calib) {
    if (protectedWrite(drv, GO, 0x01) != 0)
      return -1; // GO to start Auto-Calib process
  } else {
    while (protectedWrite(drv, MODE, 0x05) != 0) // set DRV to RTP Mode
    {
    }
    while (protectedWrite(drv, RTP_INPUT, 0x00) !=
           0) // set vibration value to 0
    {
    }
  }
  return 0;
}

//________________________________________________
// When an error occurs or the program is exited: mute the DRVs first.
void MotorBoard::muteAll() {
  for (int i = 0; i < 9; ++i) {
    drvSelect(i);
    protectedWrite(drv, RTP_INPUT, 0);
    delay(1);
  }
  delay(1);
  // printf("Muted all LRAs \n");
}

//________________________________________________
// Play an on off pattern
void MotorBoard::runOnOffPattern(int onTime, int offTime, int passes) {
  for (int i = 0; i < 9; ++i) {
    drvSelect(i);
    protectedWrite(drv, RTP_INPUT, 0);
  }
  delay(offTime);
  for (int u = 0; u < passes; ++u) {
    for (int i = 0; i < 9; ++i) {
      drvSelect(i);
      protectedWrite(drv, RTP_INPUT, 255);
    }
    delay(onTime);
    for (int i = 0; i < 9; ++i) {
      drvSelect(i);
      protectedWrite(drv, RTP_INPUT, 0);
    }
    delay(offTime);
  }
}

//________________________________________________
// Reset all DRV shields
void MotorBoard::resetAll() {
  for (int u = 0; u < 9; u++) {
    drvSelect(u);
    delay(1);
    // First: Set DEV_RESET bit to 1
    while (protectedWrite(drv, MODE, 0x80) != 0) // Do until the shield is reset
    {
      printf("Reset failed\n");
    }
  }
  delay(10);
  for (int u = 0; u < 9; u++) {
    drvSelect(u);
    delay(1);
    // Check DEV_RESET bit until it gets cleared (reset finished)
    uint8_t getMODE = 0x80;
    while ((getMODE & 0x80) != 0x00) // Do until the shield is reset
    {
      getMODE = protectedRead(drv, MODE);
      delay(1);
    }
    // Get device in active mode (end standby)
    while (protectedWrite(drv, MODE, 0x40) != 0) {
      printf("Activation failed\n");
    }
    // Check standby bit until it gets cleared (device active)
    getMODE = 0x00;
    while ((getMODE & 0x04) != 0x00) // Check until it is active
    {
      getMODE = protectedRead(drv, MODE);
      delay(1);
    }
    printf("reset actuator %i successfull\n", u);
  }
  printf("all actuators reset\n\n\n\n");
}

//________________________________________________
// print result of calibration pass
void MotorBoard::printStatusToSerial(uint8_t status) {
  printf("return:\t\t%x\ncommunication:\t", status);
  if ((status & 0xE0) == 0x00) // check if the board gives the right ID back
  {
    printf("NO ANSWER\n");
  }
  if ((status & 0xE0) != 0x00) { // if it does, check the rest of the status
    printf("CONNECTED\ncalib success:\t");
    if ((status & 0x8) == 0x00) // check is calib was successfull
      printf("YES\n");
    else {
      printf("NO\n");
      printf("overtemperature:\t");
      if ((status & 0x2) != 0x00)
        printf("YES\n");
      else
        printf("NO\n");
      printf("overcurrent:\t");
      if ((status & 0x1) != 0x00)
        printf("YES\n");
      else
        printf("NO\n");
    }
  }
  int freq = 0x00;
  freq = protectedRead(drv, LRA_RESON);
  float frequency = 1 / (freq * 0.00009846);
  printf("frequency:\t%.2f Hz\n", frequency);
}

//________________________________________________
// print calibration summary
void MotorBoard::printSummary() {
  printf("\n\n\n here are the results of the calib test-run: \n");
  printf("______________________________\n");
  for (int u = 0; u < 9; u++) {
    printf("LRA No %i: \t ", u);
    drvSelect(u);
    uint8_t getReg = 0x00;
    getReg = protectedRead(drv, A_CAL_COMP);
    printf("\t Compensation:  %u \t ", getReg);
    getReg = protectedRead(drv, A_CAL_BEMF);
    printf("\t back-EMF:  %u \t ", getReg);
    printf("\t  Device is ");
    if (calibSuccess[u] == true) {
      printf("READY\n");
      availableLRAs++;
    } else {
      printf("not ready! \n");
    }
  }
  printf("\ntotal of:\t%i\n", availableLRAs);
}

// Do the Calibration Process
void MotorBoard::runCalib() {
  resetAll();
  for (int u = 0; u < 9; u++) {
    drvSelect(u); // select the driver to write to
    while (setupLRA(true) != 0) {
      // printf("writing settings to no %i failed \n", u); //Send all register
      // settings and "GO" bit to start auto calibration
    }
  }
  // Check if autocalibration already was finished and successfull. If not, do
  // subsequent calibration passes
  for (int u = 0; u < 9; u++) {
    drvSelect(u); // select the driver to write to
    printf("\n\n\n\nDRV No: %i\n", u);
    printf("_________________\n");
    int calibCounter = 0;
    // Get GO bit - when cleared auto calibration has been finished
    uint8_t getGO = 0x01;
    while ((getGO & 0x01) != 0x00) {
      getGO = protectedRead(drv, GO);
      delay(5);
    }
    // Get status register to check if auto calibration was successfull
    uint8_t getStatus = protectedRead(drv, STATUS);
    if ((getStatus & 0x08) == 0x00) {
      calibSuccess[u] = true;
    } else {
      // Cancel when there were too many calibration passes
      while (calibCounter < maxCalibPasses) {
        // Start next calibration
        printf("LRA %i returned status %x. Calib pass no %i\n", u, getStatus,
               calibCounter);
        while (setupLRA(true) != 0) {
          printf("setup of actuator No %i failed\n",
                 u); // Send all register settings and "GO" bit to start auto
                     // calibration
        }
        delay(50);
        getGO = 0x01;
        while ((getGO & 0x01) != 0x00) {
          getGO = protectedRead(drv, GO);
          delay(5);
        }
        getStatus = protectedRead(drv, STATUS);
        if ((getStatus & 0x08) == 0x00) {
          calibSuccess[u] = true;
          break;
        }
        calibCounter++; // count the calibration passes
      }
      if (calibCounter == maxCalibPasses) {
      }
      printf("FATAL: cannot calibrate LRA No %i", u);
    }
    // If Calibration was successfull print results and switch to active mode
    if (calibSuccess[u] == true) {
      while (protectedWrite(drv, MODE, 0x05) != 0) // set DRV to RTP Mode
      {
        printf("setting No %i in RTP mode failed\n", u);
      }
      while (protectedWrite(drv, RTP_INPUT, 0x00) !=
             0) // set vibration value to 0
      {
        printf("setting RTP value at No %i to zero failed\n", u);
      }
      printStatusToSerial(getStatus);
    }
  }
  printSummary();
}

int MotorBoard::protectedRead(int addr, unsigned char ucRegAddress) {
  std::lock_guard<std::mutex> locki2c(Glob::i2cMux);
  return Glob::i2c.readReg(addr, ucRegAddress);
}
int MotorBoard::protectedWrite(int addr, unsigned char ucRegAddress,
                               char cValue) {
  std::lock_guard<std::mutex> locki2c(Glob::i2cMux);
  return Glob::i2c.writeReg(addr, ucRegAddress, cValue);
}
