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
#include <ctime>
#include <fstream>
#include <sstream>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>

//----------------------------------------------------------------------
// DECLARATIONS
//----------------------------------------------------------------------
void setupGlove();
void muteAll();
void writeValues(int valc, uint8_t *vals);

// drv board detachable?
extern bool detachableDRV;
