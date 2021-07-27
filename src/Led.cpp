// Jakob 27.07.21
// first try to get LEDs running
// on CM4 use wiringpi 2.60 (unofficial)!

//----------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------
#include "Globals.hpp"

#include <wiringPi.h>

//----------------------------------------------------------------------
// METHODS
//----------------------------------------------------------------------
void Led::init() {
  pinMode(rPin, OUTPUT);
  pinMode(gPin, OUTPUT);
  pinMode(bPin, OUTPUT);

  digitalWrite(rPin, 1);
  digitalWrite(gPin, 1);
  digitalWrite(bPin, 1);
}

void Led::setR(bool val) { digitalWrite(rPin, val); }

void Led::setG(bool val) { digitalWrite(gPin, val); }

void Led::setB(bool val) { digitalWrite(bPin, val); }

void Led::off() {
  digitalWrite(rPin, 1);
  digitalWrite(gPin, 1);
  digitalWrite(bPin, 1);
}