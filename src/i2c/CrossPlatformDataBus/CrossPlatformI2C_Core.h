/*
   CrossPlatformI2C_core.h: Declarations f core cross-platform I2C functions

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

#pragma once

#include <stdint.h>

void cpi2c_readRegisters(uint8_t address, uint8_t subAddress, uint8_t count,
                         uint8_t *dst);

bool cpi2c_writeRegister(uint8_t address, uint8_t subAddress, uint8_t data);

bool cpi2c_writeRegisters(uint8_t address, uint8_t subAddress, uint8_t count,
                          uint8_t *src);

uint8_t cpi2c_open(uint8_t address, uint8_t bus = 1);
