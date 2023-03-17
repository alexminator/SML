#include "Arduino.h"

class Comet
{
public:
  Comet(){};
  void runPattern();

private:
};

void Comet::runPattern()
{
  const byte fadeAmt = 150;
  const int cometSize = 3;
  const int deltaHue = 4;
  const double cometSpeed = 0.5;

  static byte hue = HUE_RED;        //Current Color
  static int iDirection = 1;        //Current direction (-1 or +1)
  static double iPos = 0.0;         //Current comet position

  hue += deltaHue;                  //Update comet color
  iPos += iDirection * cometSpeed;  //Update comet position

  if (iPos == (N_PIXELS - cometSize) || iPos == 0)
    iDirection *= -1;

  for (int i = 0; i < cometSize; i++)
    leds[(int)iPos + i].setHue(hue);

  // Randomly fade the LEDs
  for (int j = 0; j < N_PIXELS; j++)
    if (random(2) == 1)
      leds[j] = leds[j].fadeToBlackBy(fadeAmt);

  FastLED.show();
  delay(50);
}
