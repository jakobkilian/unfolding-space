#pragma once

//****************************************************************
//                          LED Class
//****************************************************************

class Led {
public:
  Led(int thisrPin, int thisgPin, int thisbPin) {
    rPin = thisrPin;
    gPin = thisgPin;
    bPin = thisbPin;
  }
  void init();
  void setR(bool);
  void setG(bool);
  void setB(bool);
  void setDimR(bool);
  void setDimG(bool);
  void setDimB(bool);
  void off();

private:
  int rPin;
  int gPin;
  int bPin;
};