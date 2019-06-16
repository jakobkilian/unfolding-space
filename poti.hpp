#pragma once
#include <usb.h>        // this is libusb, see http://libusb.sourceforge.net/

void initPoti();
int updatePoti();

extern int globalPotiVal;
extern bool potiAv;
