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

void Led::setR(bool val) {
  // dirty hack: set back to Output before writing
  pinMode(rPin, OUTPUT);
  digitalWrite(rPin, !val);
}

void Led::setG(bool val) {
  // dirty hack: set back to Output before writing
  pinMode(gPin, OUTPUT);
  digitalWrite(gPin, !val);
}

void Led::setB(bool val) {
  // dirty hack: set back to Output before writing
  pinMode(bPin, OUTPUT);
  digitalWrite(bPin, !val);
}

void Led::setDimR(bool val) {
  // dirty hack: use INPUT pullup to make dim light
  if (!val) {
    pinMode(rPin, OUTPUT);
    digitalWrite(rPin, 1);
  } else {
    pinMode(rPin, INPUT);
  }
}

void Led::setDimG(bool val) {
  // dirty hack: use INPUT pullup to make dim light
  if (!val) {
    pinMode(gPin, OUTPUT);
    digitalWrite(gPin, 1);
  } else {
    pinMode(gPin, INPUT);
  }
}

void Led::setDimB(bool val) {
  // dirty hack: use INPUT pullup to make dim light
  if (!val) {
    pinMode(bPin, OUTPUT);
    digitalWrite(bPin, 1);
  } else {
    pinMode(bPin, INPUT);
  }
}

void Led::off() {
  // dirty hack: set back to Output before writing
  pinMode(rPin, OUTPUT);
  pinMode(gPin, OUTPUT);
  pinMode(gPin, OUTPUT);
  digitalWrite(rPin, 1);
  digitalWrite(gPin, 1);
  digitalWrite(bPin, 1);
}