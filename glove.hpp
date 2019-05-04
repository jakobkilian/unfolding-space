#pragma once

#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <fstream>
#include <sstream>
#include <ctime>

void setupGlove();
void muteAll();
void writeValues(int valc, uint8_t*vals);

