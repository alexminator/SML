class Temperature {
public:
  Temperature() {};
  void runPattern();
private:
};

void Temperature::runPattern()
{
  FastLED.clear();
  int t = map(temp, 17, 40, 0, N_PIXELS);  // ! Range 17-40 degree are 24 = N_PIXELS. One led for degree

  for (uint16_t L = 0; L < t; L++)
  {
    leds[L] = wheel(((205 + (L * 6)) & 255)); // Gradient from blue (cold) to green (ok) to red (warm),first value here 205 = start color
                                              // ! It is multiplied by 3 for 50 LEDs, by 6 for 24 and by 9 for 16 LEDs.
  }
  FastLED.show();
}
