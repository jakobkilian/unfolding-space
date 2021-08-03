#include "MMC5633.h"
#include "Globals.hpp"
#include <math.h>

MMC5633::MMC5633() {
  // Construct
}

void MMC5633::begin() {
  seti2cAddr();
  uint8_t id = readRegister(0x39);
  printf("MMC ID is: %i\n", id);
}

uint8_t MMC5633::readRegister(uint8_t subAddress) {
  uint8_t temp;
  readRegisters(subAddress, 1, &temp);
  return temp;
}

void MMC5633::seti2cAddr() {
  std::lock_guard<std::mutex> locki2c(Glob::i2cMux);
  i2cAddr = (uint8_t)Glob::i2c.setupDevice(ADDRESS);
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
