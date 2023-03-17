#include "Arduino.h"

class Juggle {
  public:
    Juggle(){};
    void runPattern();
  private:
};



void Juggle::runPattern() { 
  const uint8_t NUM_DOTS = 4; // Number of dots in use.
  const uint8_t HUE_INC = 16; // Incremental change in hue between each dot.
  static uint8_t thishue = 0; // Starting hue.
  static uint8_t curhue = 0;
  const uint8_t basebeat = 5;
  
  curhue = thishue; // Reset the hue values?
  fadeToBlackBy(leds, N_PIXELS, FADE_RATE);
  for (int i = 0; i < NUM_DOTS; i++) {
    leds[beatsin16(basebeat + i + NUM_DOTS, 0, N_PIXELS - 1)] |= CHSV(curhue, 255, BRIGHTNESS);
    curhue += HUE_INC;
  }

  FastLED.show();
}
