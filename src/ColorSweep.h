// WLED-based Color Sweep effect - wipes color, turns off opposite
// Adapted from WLED mode_color_sweep implementation
class ColorSweep {
  public:
    ColorSweep(){};
    void runPattern();
  private:
};

void ColorSweep::runPattern() {
  // WLED-compatible parameter
  extern uint8_t sweepSpeed;    // Default: 128, range: 0-255 (sweep speed)

  static uint16_t index = 0;
  static bool forward = true;
  static unsigned long lastTime = 0;

  unsigned long currentTime = millis();
  uint32_t cycleTime = (256 - sweepSpeed) * 2;

  if (currentTime - lastTime >= cycleTime) {
    lastTime = currentTime;

    // Turn off previous pixel
    int prevIndex = forward ? (index - 1 + N_PIXELS) % N_PIXELS : (index + 1) % N_PIXELS;
    leds[prevIndex] = CRGB::Black;

    // Set current pixel
    leds[index] = CHSV(myhue, 255, brightness);

    // Move to next pixel
    if (forward) {
      index++;
      if (index >= N_PIXELS) {
        index = N_PIXELS - 1;
        forward = false;
        myhue++;  // Change color on direction change
      }
    } else {
      if (index == 0) {
        index = 0;
        forward = true;
        myhue++;  // Change color on direction change
      } else {
        index--;
      }
    }
  }

  FastLED.show();
  vTaskDelay(pdMS_TO_TICKS(20));  // 50 FPS
}
