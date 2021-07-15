/*
   WiringPiI2C.cpp: WiringPi implementation of cross-platform I2C routines

   This file is part of CrossPlatformDataBus.

   CrossPlatformDataBus is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   CrossPlatformDataBus is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with CrossPlatformDataBus.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "CrossPlatformI2C.h"

#include "Globals.hpp"
#include <wiringPi.h>
#include <wiringPiI2C.h>

uint8_t cpi2c_open(uint8_t address, uint8_t bus) {
  std::lock_guard<std::mutex> locki2c(Glob::i2cMux);
  (void)bus;
  return (uint8_t)Glob::i2c.setupDevice(address);
}

uint16_t cpi2c_readRegister_8_16(uint8_t address, uint8_t subAddress) {
  std::lock_guard<std::mutex> locki2c(Glob::i2cMux);
  return Glob::i2c.readReg16(address, subAddress);
}

void cpi2c_readRegisters(uint8_t address, uint8_t subAddress, uint8_t count,
                         uint8_t *dst) {
  std::lock_guard<std::mutex> locki2c(Glob::i2cMux);
  for (uint8_t k = 0; k < count; ++k) {
    dst[k] = Glob::i2c.readReg(address, subAddress + k);
  }
}

bool cpi2c_writeRegister(uint8_t address, uint8_t subAddress, uint8_t data) {
  std::lock_guard<std::mutex> locki2c(Glob::i2cMux);
  return Glob::i2c.writeReg(address, subAddress, data) == 0;
}

bool cpi2c_writeRegister_16_8(uint8_t address, uint16_t subAddress,
                              uint8_t data) {
  std::lock_guard<std::mutex> locki2c(Glob::i2cMux);
  return Glob::i2c.writeReg16(address, subAddress, data) > 0;
}

bool cpi2c_writeRegisters(uint8_t address, uint8_t subAddress, uint8_t count,
                          uint8_t *src) {
  std::lock_guard<std::mutex> locki2c(Glob::i2cMux);
  for (uint8_t k = 0; k < count; ++k) {
    if (!cpi2c_writeRegister(address, subAddress + k, src[k])) {
      return false;
    }
  }

  return true;
}
