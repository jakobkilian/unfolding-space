#include <DigiUSB.h>

byte avSize = 15;
int avSum;
int av;
int oldav;

void setup()
{
  DigiUSB.begin();
}

void loop()
{
  avSum = 0;    //reset  average sum
  for (int i = 0; i < avSize; i++) {
    avSum = avSum + analogRead(1);  //add 15 (avSize) values to the average array
    delay(5);                       //wait some time before takeing next value
  }

  //calc the average
  av = map(avSum / 15.0, 3, 1020,99, 0);

  //print it if it is different to the last printed
  if (av != oldav) {
    DigiUSB.println(av);
  }
  oldav = av;

  DigiUSB.delay(10);
}
