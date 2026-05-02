// WLED-based Theater Chase effect - theatre-style crawling lights
// Inspired by Adafruit examples, adapted from WLED mode_theater_chase
class TheaterChase {
  public:
    TheaterChase(){};
    void runPattern();
  private:
};

void TheaterChase::runPattern() {
  // WLED-compatible parameters
  extern uint8_t theaterSpeed;  // Default: 128, range: 0-255 (animation speed)
  extern uint8_t theaterGap;    // Default: 3, range: 1-10 (gap size between lights)

  static int step = 0;
  static unsigned long lastTime = 0;

  // WLED algorithm: time-based stepping
  unsigned long currentTime = millis();
  uint32_t cycleTime = (256 - theaterSpeed) * 5;

  if (currentTime - lastTime >= cycleTime) {
    lastTime = currentTime;

    // Clear all LEDs
    fill_solid(leds, N_PIXELS, CRGB::Black);

    // Draw theater chase pattern
    for (int i = 0; i < N_PIXELS; i++) {
      if ((i + step) % (theaterGap + 1) == 0) {
        leds[i] = CHSV(myhue, 255, brightness);
      }
    }

    step = (step + 1) % (theaterGap + 1);

    // Cycle through colors slowly
    if (step == 0) {
      myhue++;
    }
  }

  FastLED.show();
  vTaskDelay(pdMS_TO_TICKS(20));  // 50 FPS
}
