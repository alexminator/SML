
// External parameters defined in main.cpp
extern uint8_t sinelonBeat;
extern uint8_t sinelonFade;

class Sinelon {
  public:
    Sinelon(){};
    void runPattern();
  private:
};

void Sinelon::runPattern() {
  fadeToBlackBy(leds, N_PIXELS, sinelonFade);
  int pos1 = beatsin16(sinelonBeat, 0, N_PIXELS - 1);
  int pos2 = beatsin16(sinelonBeat + 5, 0, N_PIXELS - 1);
  leds[(pos1 + pos2) / 2] += CHSV(myhue, 255, brightness);

  EVERY_N_MILLISECONDS(10) {
    myhue++;
  }

  FastLED.show();
}
