// WLED-based Color Wipe effect - lights all LEDs one after another
// Adapted from WLED mode_color_wipe implementation
class ColorWipe {
  public:
    ColorWipe(){};
    void runPattern();
  private:
};

void ColorWipe::runPattern() {
  // WLED-compatible parameters
  extern uint8_t wipeSpeed;    // Default: 128, range: 0-255 (wipe speed)
  extern bool wipeReverse;     // Default: false (true = wipe backwards)

  static uint16_t index = 0;
  static unsigned long lastTime = 0;

  // WLED algorithm: time-based stepping
  unsigned long currentTime = millis();
  uint32_t cycleTime = (256 - wipeSpeed) * 2;  // Speed to delay mapping

  if (currentTime - lastTime >= cycleTime) {
    lastTime = currentTime;

    // Clear previous pixel if reverse mode
    if (wipeReverse) {
      if (index > 0) {
        leds[index - 1] = CRGB::Black;
      } else {
        leds[N_PIXELS - 1] = CRGB::Black;
      }
    }

    // Set current pixel
    leds[index] = CHSV(myhue, 255, brightness);

    // Move to next pixel
    if (wipeReverse) {
      index = (index > 0) ? index - 1 : N_PIXELS - 1;
    } else {
      index = (index + 1) % N_PIXELS;
    }

    // Cycle through colors on each complete wipe
    if (index == 0) {
      myhue++;
    }
  }

  FastLED.show();
  vTaskDelay(pdMS_TO_TICKS(20));  // 50 FPS
}
