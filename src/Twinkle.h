
class Twinkle {
  public:
    Twinkle(){};
    void runPattern();
  private:
};

void Twinkle::runPattern() {
  
  if (random(25) == 1) {
    uint16_t i = random(N_PIXELS);
    //leds[i] = CRGB(random(256), random(256), random(256));
    leds[i] = CHSV(random(256), 255, brightness);
  }
  fadeToBlackBy(leds, N_PIXELS, FADE_RATE);

  FastLED.show();
  delay(10);
}
