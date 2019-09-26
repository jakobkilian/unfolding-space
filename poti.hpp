/*
 * File: poti.hpp
 * Author: Jakob Kilian
 * Date: 26.09.19
 * Project: unfoldingspace.jakobkilian.de
 */

#pragma once

//----------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------
#include <usb.h> // this is libusb, see http://libusb.sourceforge.net/

//----------------------------------------------------------------------
// DECLARATIONS
//----------------------------------------------------------------------
void initPoti();
int updatePoti();

extern int globalPotiVal;
extern bool potiAv;
