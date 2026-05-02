// WLED-based Dissolve effect - random pixels until full
// Adapted from WLED mode_dissolve implementation
class Dissolve {
  public:
    Dissolve(){};
    void runPattern();
  private:
};

void Dissolve::runPattern() {
  // WLED-compatible parameters
  extern uint8_t dissolveSpeed;    // Default: 128, range: 0-255 (dissolve speed)
  extern bool dissolveRandom;      // Default: false (true = random colors)

  static uint8_t dissolvedCount = 0;
  static bool filled = false;
  static unsigned long lastTime = 0;

  unsigned long currentTime = millis();
  uint32_t cycleTime = (256 - dissolveSpeed) * 2;

  if (currentTime - lastTime >= cycleTime) {
    lastTime = currentTime;

    // Reset when all LEDs are lit
    if (filled) {
      fill_solid(leds, N_PIXELS, CRGB::Black);
      dissolvedCount = 0;
      filled = false;
      myhue++;
    } else {
      // Add random pixels
      uint8_t pixelsToAdd = map(dissolveSpeed, 0, 255, 1, 5);

      for (uint8_t i = 0; i < pixelsToAdd; i++) {
        if (dissolvedCount < N_PIXELS) {
          uint16_t pos = random16(N_PIXELS);

          if (leds[pos]) {
            // Already lit, try another
            continue;
          }

          if (dissolveRandom) {
            leds[pos] = CHSV(random8(), 255, brightness);
          } else {
            leds[pos] = CHSV(myhue, 255, brightness);
          }

          dissolvedCount++;
        }
      }

      if (dissolvedCount >= N_PIXELS) {
        filled = true;
      }
    }
  }

  FastLED.show();
  vTaskDelay(pdMS_TO_TICKS(20));  // 50 FPS
}
