#include "MMC5633.h"
#include "Globals.hpp"
#include <math.h>

MMC5633::MMC5633() {
  // Construct
}

void MMC5633::begin() {
  seti2cAddr();
  uint8_t id = readRegister(Product_ID);
  printf("MMC ID is: %i\n", id);
  // setup continuous mode
  writeRegister(ODR,
                75); // 75 is the fastest freq with default internal Ctrl 1 /BW
  writeRegister(CTRL_0, 0b10100000); // cmm_freq_en | auto set/reset
  writeRegister(CTRL_2, 0b00011000); // cmm_en | en_prd_set
}

uint8_t MMC5633::readRegister(uint8_t subAddress) {
  uint8_t temp;
  readRegisters(subAddress, 1, &temp);
  return temp;
}

// setup wiringPi handler.
void MMC5633::seti2cAddr() {
  std::lock_guard<std::mutex> locki2c(Glob::i2cMux);
  i2cAddr = (uint8_t)Glob::i2c.setupDevice(ADDRESS);
}

int MMC5633::pollX() {
  uint8_t x[3] = {0};
  readRegisters(Xout0, 2, x);
  readRegisters(Xout2, 1, x + 2);
  uint32_t res = (x[0] << 16) | (x[1] << 8) | x[2];
  return res;
}

int MMC5633::pollY() {
  uint8_t y[3] = {0};
  readRegisters(Yout0, 2, y);
  readRegisters(Yout2, 1, y + 2);
  uint32_t res = (y[0] << 16) | (y[1] << 8) | y[2];
  return res;
}

int MMC5633::pollZ() {
  uint8_t z[3] = {0};
  readRegisters(Zout0, 2, z);
  readRegisters(Zout2, 1, z + 2);
  uint32_t res = (z[0] << 16) | (z[1] << 8) | z[2];
  return res;
}

void MMC5633::writeRegister(uint8_t subAddress, uint8_t data) {
  std::lock_guard<std::mutex> locki2c(Glob::i2cMux);
  Glob::i2c.writeReg(i2cAddr, subAddress, data);
}

void MMC5633::readRegisters(uint8_t subAddress, uint8_t count, uint8_t *dest) {
  std::lock_guard<std::mutex> locki2c(Glob::i2cMux);
  for (uint8_t k = 0; k < count; ++k) {
    dest[k] = Glob::i2c.readReg(i2cAddr, subAddress + k);
  }
}
