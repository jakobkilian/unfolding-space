/*
 * File: glove.hpp
 * Author: Jakob Kilian
 * Date: 26.09.19
 * Project: unfoldingspace.jakobkilian.de
 */

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

//----------------------------------------------------------------------
// DECLARATIONS
//----------------------------------------------------------------------
void setupGlove();
void muteAll();
void sendValuesToGlove(unsigned char values[],int size);
void testMotorNo(int i, int val);
void doCalibration();
// drv board detachable?
extern bool detachableDRV;
