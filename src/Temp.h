class Temperature {
public:
  Temperature() {};
  void runPattern();
private:
};

CRGB wheel(byte WheelPos)
{

  if (WheelPos < 85)
  {
    return CRGB(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  else if (WheelPos < 205)
  {
    WheelPos -= 85;
    return CRGB(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  else
  {
    WheelPos -= 205;
    return CRGB(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

void Temperature::runPattern()
{
  FastLED.clear();
  int t = map(temp, 17, 40, 0, N_PIXELS);  // Range 17-40 degree are 24 = N_PIXELS. One led for degree

  for (uint16_t L = 0; L < t; L++)
  {
    leds[L] = wheel(((205 + (L * 6)) & 255)); // Gradient from blue (cold) to green (ok) to red (warm), first value here 205 = start color, L = 3 for 50 leds. L = 9 for 16 leds
  }
  FastLED.show();
}
