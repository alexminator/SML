class temperature
{
public:
  TempNeo() {};
  void runPattern();

private:
};

CRGB wheel(byte WheelPos)
{
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)
  {
    return CRGB(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  else if (WheelPos < 170)
  {
    WheelPos -= 85;
    return CRGB(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  else
  {
    WheelPos -= 170;
    return CRGB(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
};

void TempNeo::runPattern()
{
  int t = map(temp, 20, 70, 0, N_PIXELS);

  for (uint16_t L = 0; L < t; L++)
  {
    leds[L] = wheel(((205 + (L * 3)) & 255)); // Gradient from blue (cold) to green (ok) to red (warm), first value here 205 = start color, second value here 42 = end color
  }
  FastLED.show();
  delay(50);
};
