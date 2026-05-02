
// External parameters defined in main.cpp
extern uint8_t fireCooling;
extern uint8_t fireSparking;
extern bool fireReverse;

class Fire {
  public:
    Fire(){};
    void runPattern();
  private:
};

void Fire::runPattern() {
  // Array of temperature readings at each simulation cell
  static byte heat[N_PIXELS];

  // Step 1.  Cool down every cell a little
  for (int i = 0; i < N_PIXELS; i++) {
    heat[i] = qsub8(heat[i], random8(0, ((fireCooling * 10) / N_PIXELS) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for (int k = N_PIXELS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if (random8() < fireSparking) {
    int y = random8(7);
    heat[y] = qadd8(heat[y], random8(160, 255));
  }

  // Step 4.  Map from heat cells to LED colors
  for (int j = 0; j < N_PIXELS; j++) {
    // Scale the heat value from 0-255 down to 0-240
    // for best results with color palettes.
    byte colorindex = scale8(heat[j], 240);
    CRGB color = ColorFromPalette(CRGBPalette16(CRGB::Black, CRGB::Red, CRGB::Yellow, CRGB::White), colorindex);

    // Use reverse mode if enabled
    int pixelnumber = fireReverse ? (N_PIXELS - 1 - j) : j;
    leds[pixelnumber] = color;
  }

  FastLED.show();
}
