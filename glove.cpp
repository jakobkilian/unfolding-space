/*
 * File: glove.cpp
 * Author: Jakob Kilian
 * Date: 26.09.19
 * Info: I2C communication with TCA9548A and DRV2605, controlling the motors
 * Project: unfoldingspace.jakobkilian.de
 */

//----------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------
#include "glove.hpp"

#include "camera.hpp"
#include "globals.hpp"
#include "timelog.hpp"

#include <iostream>

//----------------------------------------------------------------------
// DECLARATION OF FUNCTIONS
//----------------------------------------------------------------------
int registerWrite(unsigned char, char);
uint8_t registerRead(unsigned char);
int drvSelect(uint8_t);
int setupLRA(bool);
void myi2cSetup();
void calibAll();
void resetAll();
void printStatusToSerial(uint8_t);
void printSummary();

//----------------------------------------------------------------------
// DEFINE ADRESSSES
//----------------------------------------------------------------------
// Adresses on the DRV2605 Register
#define STATUS 0x00
#define MODE 0x01
#define RTP_INPUT 0x02
#define LIB_SEL 0x03
#define WAV_SEQ1 0x04
#define WAV_SEQ2 0x05
#define WAV_SEQ3 0x06
#define WAV_SEQ4 0x07
#define WAV_SEQ5 0x08
#define WAV_SEQ6 0x09
#define WAV_SEQ7 0x0A
#define WAV_SEQ8 0x0B
#define GO 0x0C
#define ODT_OFFSET 0x0D
#define SPT 0x0E
#define SNT 0x0F
#define BRT 0x10
#define ATV_CON 0x11
#define ATV_MIN_IN 0x12
#define ATV_MAX_IN 0x13
#define ATV_MIN_OUT 0x14
#define ATV_MAX_OUT 0x15
#define RATED_VOLTAGE 0x16
#define OD_CLAMP 0x17
#define A_CAL_COMP 0x18
#define A_CAL_BEMF 0x19
#define FB_CON 0x1A
#define CONTRL1 0x1B
#define CONTRL2 0x1C
#define CONTRL3 0x1D
#define CONTRL4 0x1E
#define VBAT_MON 0x21
#define LRA_RESON 0x22
// I2C Adresses of TCA9548A and DRV2605
#define DRV2605_ADDRESS 0x5A
#define TCA9548A_0_ADDRESS 0x70
#define TCA9548A_1_ADDRESS 0x72

//----------------------------------------------------------------------
// DEFINE ACTUATORS STRENGTH CURVE
//----------------------------------------------------------------------
// Vibraiton actuator doesn't have a linear course
uint8_t altCurve[] = {
    0,   20,  20,  20,  21,  21,  21,  21,  21,  21,  21,  22,  22,  22,  22,
    22,  22,  23,  23,  23,  23,  24,  24,  24,  24,  25,  25,  25,  25,  26,
    26,  26,  27,  27,  28,  28,  28,  29,  29,  30,  30,  31,  31,  32,  32,
    33,  33,  34,  34,  35,  36,  36,  37,  37,  38,  39,  39,  40,  40,  41,
    42,  42,  43,  44,  44,  45,  45,  46,  47,  47,  48,  49,  49,  50,  51,
    51,  52,  53,  54,  54,  55,  56,  56,  57,  58,  59,  59,  60,  61,  61,
    62,  63,  64,  64,  65,  66,  67,  67,  68,  69,  70,  70,  71,  72,  73,
    74,  74,  75,  76,  77,  78,  78,  79,  80,  81,  82,  82,  83,  84,  85,
    86,  87,  87,  88,  89,  90,  91,  92,  93,  93,  94,  95,  96,  97,  98,
    99,  100, 101, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112,
    113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
    128, 129, 130, 131, 132, 133, 134, 135, 136, 138, 139, 140, 141, 142, 143,
    144, 145, 146, 148, 149, 150, 151, 152, 153, 154, 155, 157, 158, 159, 160,
    161, 162, 164, 165, 166, 167, 168, 169, 171, 172, 173, 174, 175, 177, 178,
    179, 180, 181, 183, 184, 185, 186, 187, 189, 190, 191, 192, 194, 195, 196,
    197, 198, 200, 201, 202, 203, 205, 206, 207, 208, 210, 211, 212, 213, 215,
    216, 217, 218, 220, 221, 222, 224, 225, 227, 229, 232, 235, 239, 245, 254};

//----------------------------------------------------------------------
// VARIABLES
//----------------------------------------------------------------------
uint8_t asciiArt[] = {'.', '-', '+', 'x', 'o', 'O'};
uint8_t vibVal[9];           // current vibration strength of the motors
uint8_t lastVibVal[9];       // last vibration strength
uint8_t maxCalibPasses = 2;  // max trys for calib before skipping
uint8_t availableLRAs = 0;   // number of LRAs
bool calibSuccess[9];        // was calibration successfull?
int retVal;
int lastTCA;
uint8_t order[9] = {5, 8, 2, 6, 0, 3, 7, 1, 4};

// Should there be a calibration in the beginning? Else: Take standard Values
bool startupCalib = false;

// respective I2C Address
int drv;
int tca0;
int tca1;

//----------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------

//________________________________________________
// In WiringPi Lib every I2C Device has to be init to get a respective address:
uint8_t initI2CDevice(uint8_t addr) {
  int respectiveAddr = wiringPiI2CSetup(addr);
  if (respectiveAddr < 0) {
    printf("I2CSetup Failed, %i\n", respectiveAddr);
  }
  // else {
  // printf("I2CSetup successfull: %i \n", respectiveAddr);
  //}
  return respectiveAddr;
}

//________________________________________________
// initially set up all TCA9548A, DRV2605 and the actuators
void setupGlove() {
  wiringPiSetup();
  drv = initI2CDevice(DRV2605_ADDRESS);
  tca0 = initI2CDevice(TCA9548A_0_ADDRESS);
  tca1 = initI2CDevice(TCA9548A_1_ADDRESS);
  // resetAll(); //to stop ongoing vibrations or faulty settings
  // Write settings to all drivers and start simultaneous auto calibration
  for (int u = 0; u < 9; u++) {
    drvSelect(u);  // select the driver to write to
    while (setupLRA(startupCalib) != 0) {
      // printf("writing settings to no %i failed \n", u); //Send all register
      // settings and "GO" bit to start auto calibration
    }
  }
  // the following calibration process can be skipped (startupCalib=false).
  // The standard values do their job well enough
  if (startupCalib) {
    doCalibration();
  }
}

//________________________________________________
// Read data from a register
uint8_t registerRead(unsigned char ucRegAddress) {
  int data;
  if ((data = wiringPiI2CReadReg8(drv, ucRegAddress)) < 0) {
    printf("failed reading register:   ");
    printf("%i\n", data);
    return -1;
  }
  delayMicroseconds(100);
  return data;
}


//________________________________________________
// Write data to a register
int registerWrite(unsigned char ucRegAddress, char cValue) {
  int data;
  if ((data = wiringPiI2CWriteReg8(drv, ucRegAddress, cValue)) != 0) {
    printf("failed writing to register:   addr=%u, res=%i\n", ucRegAddress,
           data);
    return -1;
  }
  // delayMicroseconds(100);
  return 0;
}

void sendValuesToGlove(unsigned char inValues[], int size) {
  // WRITE VALUES TO GLOVE
  unsigned char values[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
  glob::logger.mainLogger.store("startSendGlove");
  {
    for (int i = 0; i < size; i++) {
      values[i] = inValues[i];
    }
  }
  glob::logger.mainLogger.store("copy");
  // Write Values to the registers of the motor drivers (drv..)
  // All drv have same addr. -> two i2c multiplexer (tca) are needed.
  if (!glob::modes.a_muted && !glob::royalStats.a_isCalibRunning) {
    // For speed's sake start with drvs that are on the first tca
    for (int i = 0; i < size; ++i) {
      if (order[i] <= 4) {
        drvSelect(order[i]);  // route the value to the right TCA and DRV
        registerWrite(RTP_INPUT, altCurve[values[i]]);
      }
    }
    glob::logger.mainLogger.store("TCA1");
    // Now all drv on the 2nd tca together
    for (int i = 0; i < size; ++i) {
      if (order[i] > 4) {
        drvSelect(order[i]);  // route the value to the right TCA and DRV
        registerWrite(RTP_INPUT, altCurve[values[i]]);
      }
    }
  }
  glob::logger.mainLogger.store("TCA2");
  glob::logger.mainLogger.store("end");
  //glob::logger.mainLogger.printAll("Cycle", "us", "ms");
  glob::logger.mainLogger.udpTimeSpan("processing", "us", "startProcess", "endProcess");
  glob::logger.mainLogger.udpTimeSpan("onNewData", "us", "start", "notifyProcessing");
  glob::logger.mainLogger.udpTimeSpan("gloveSending", "us", "startSendGlove", "end");
  glob::logger.mainLogger.udpTimeSpan("wholeCycle", "us", "start", "end");
  glob::logger.mainLogger.reset();
  glob::logger.mainLogger.store("startPause");
}

//________________________________________________
// Route DRV 0-4 to first TCA multiplexer and 5-8 to the second TCA
int drvSelect(uint8_t i) {
  if (i <= 4) {
    uint8_t regVal = 1 << i;
    retVal = wiringPiI2CWrite(tca0, regVal);
    if (retVal < 0) {
      printf("can't connect to tca0 in 0-4:  %i\n", retVal);
      return -1;
    }
    if (lastTCA != 0) {
      retVal = wiringPiI2CWrite(tca1, 0);
      if (retVal < 0) {
        printf("can't connect to tca1 in 0-4:  %i\n", retVal);
        return -2;
      }
    }
    lastTCA = 0;

  } else if (i > 4) {
    uint8_t regVal = 1 << (i - 5);
    retVal = wiringPiI2CWrite(tca1, regVal);
    if (retVal < 0) {
      printf("can't connect to tca0 in 5-8:  %i\n", retVal);
      return -3;
    }
    if (lastTCA != 1) {
      retVal = wiringPiI2CWrite(tca0, 0);
      if (retVal < 0) {
        printf("can't connect to tca1 in 5-8:  %i\n", retVal);
        return -4;
      }
    }
    lastTCA = 1;
  }
  return 0;
}

//________________________________________________
// Set the settings of the DRVs by writing to their registers
int setupLRA(bool calib) {
  if (registerWrite(MODE, 0x07) != 0)
    return -1;  // Device ready (no standby) | Auto calibration mode
  if (registerWrite(FB_CON, 0xA6) != 0)
    return -1;  // Mode = LRA | Brake Factor = 3x | Loop Gain = Medium | BEMF
                // Gain = 15x
  if (registerWrite(RATED_VOLTAGE, 0x46) != 0)
    return -1;  // Rated Voltage Value of 70 - Calcuclation explained in
                // datasheet p.24
  // if (registerWrite(OD_CLAMP, 0x5A) != 0) return -1;     //Overdrive Clamp
  // Value of 90 (a little more than rms cause there is no val in LRAs
  // datasheet)
  // - Calcuclation explained in datasheet p.25
  if (registerWrite(CONTRL1, 0x90) != 0)
    return -1;  // Drive Time Value of 16 (235Hz/1000/2 - 0.5 *10) -
                // Calcuclation explained in datasheet p.24
  if (registerWrite(CONTRL2, 0x75) != 0)
    return -1;  // Unidirectional Input Mode on
  if (registerWrite(CONTRL3, 0xE2) != 0)
    return -1;  // Closed/Auto-resonance Mode on
  if (registerWrite(CONTRL4, 0x30) != 0) return -1;  // Calib length of 500ms
  if (calib) {  // only when there should be a calib at startup
    if (registerWrite(GO, 0x01) != 0)
      return -1;  // GO to start Auto-Calib process
  } else {
    while (registerWrite(MODE, 0x05) != 0)  // set DRV to RTP Mode
    {
    }
    while (registerWrite(RTP_INPUT, 0x00) != 0)  // set vibration value to 0
    {
    }
  }
  return 0;
}

//________________________________________________
// When an error occurs or the program is exited: mute the DRVs first.
void muteAll() {
  for (int i = 0; i < 9; ++i) {
    drvSelect(i);
    registerWrite(RTP_INPUT, 0);
    delay(1);
  }
  delay(10);
  // printf("Muted all LRAs \n");
}

//________________________________________________
// Reset all DRV shields
void resetAll() {
  for (int u = 0; u < 9; u++) {
    drvSelect(u);
    delay(1);
    // First: Set DEV_RESET bit to 1
    while (registerWrite(MODE, 0x80) != 0)  // Do until the shield is reset
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
    while ((getMODE & 0x80) != 0x00)  // Do until the shield is reset
    {
      getMODE = registerRead(MODE);
      delay(1);
    }
    // Get device in active mode (end standby)
    while (registerWrite(MODE, 0x40) != 0) {
      printf("Activation failed\n");
    }
    // Check standby bit until it gets cleared (device active)
    getMODE = 0x00;
    while ((getMODE & 0x04) != 0x00)  // Check until it is active
    {
      getMODE = registerRead(MODE);
      delay(1);
    }
    printf("reset actuator %i successfull\n", u);
  }
  printf("all actuators reset\n\n\n\n");
}

//________________________________________________
// print result of calibration pass
void printStatusToSerial(uint8_t status) {
  printf("return:\t\t%x\ncommunication:\t", status);
  if ((status & 0xE0) == 0x00)  // check if the board gives the right ID back
  {
    printf("NO ANSWER\n");
  }
  if ((status & 0xE0) != 0x00) {  // if it does, check the rest of the status
    printf("CONNECTED\ncalib success:\t");
    if ((status & 0x8) == 0x00)  // check is calib was successfull
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
  freq = registerRead(LRA_RESON);
  float frequency = 1 / (freq * 0.00009846);
  printf("frequency:\t%.2f Hz\n", frequency);
}

//________________________________________________
// print calibration summary
void printSummary() {
  printf("\n\n\n here are the results of the calib test-run: \n");
  printf("______________________________\n");
  for (int u = 0; u < 9; u++) {
    printf("LRA No %i: \t ", u);
    drvSelect(u);
    uint8_t getReg = 0x00;
    getReg = registerRead(A_CAL_COMP);
    printf("\t Compensation:  %u \t ", getReg);
    getReg = registerRead(A_CAL_BEMF);
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
void doCalibration() {
  resetAll();
  for (int u = 0; u < 9; u++) {
    drvSelect(u);  // select the driver to write to
    while (setupLRA(true) != 0) {
      // printf("writing settings to no %i failed \n", u); //Send all register
      // settings and "GO" bit to start auto calibration
    }
  }
  // Check if autocalibration already was finished and successfull. If not, do
  // subsequent calibration passes
  for (int u = 0; u < 9; u++) {
    drvSelect(u);  // select the driver to write to
    printf("\n\n\n\nDRV No: %i\n", u);
    printf("_________________\n");
    int calibCounter = 0;
    // Get GO bit - when cleared auto calibration has been finished
    uint8_t getGO = 0x01;
    while ((getGO & 0x01) != 0x00) {
      getGO = registerRead(GO);
      delay(5);
    }
    // Get status register to check if auto calibration was successfull
    uint8_t getStatus = registerRead(STATUS);
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
                 u);  // Send all register settings and "GO" bit to start auto
                      // calibration
        }
        delay(50);
        getGO = 0x01;
        while ((getGO & 0x01) != 0x00) {
          getGO = registerRead(GO);
          delay(5);
        }
        getStatus = registerRead(STATUS);
        if ((getStatus & 0x08) == 0x00) {
          calibSuccess[u] = true;
          break;
        }
        calibCounter++;  // count the calibration passes
      }
      if (calibCounter == maxCalibPasses) {
      }
      printf("FATAL: cannot calibrate LRA No %i", u);
    }
    // If Calibration was successfull print results and switch to active mode
    if (calibSuccess[u] == true) {
      while (registerWrite(MODE, 0x05) != 0)  // set DRV to RTP Mode
      {
        printf("setting No %i in RTP mode failed\n", u);
      }
      while (registerWrite(RTP_INPUT, 0x00) != 0)  // set vibration value to 0
      {
        printf("setting RTP value at No %i to zero failed\n", u);
      }
      printStatusToSerial(getStatus);
    }
  }
  printSummary();
}