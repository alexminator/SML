// WLED-based Running Lights effect - smooth sine wave transition
// Adapted from WLED mode_running_lights implementation
class RunningLights {
  public:
    RunningLights(){};
    void runPattern();
  private:
};

void RunningLights::runPattern() {
  // WLED-compatible parameters
  extern uint8_t runningSpeed;   // Default: 128, range: 0-255 (wave speed)
  extern uint8_t runningWidth;   // Default: 3, range: 1-8 (wave width)

  static uint16_t counter = 0;

  // WLED algorithm: smooth sine wave
  counter += (runningSpeed >> 2) + 1;

  byte waveWidth = runningWidth << 3;  // Scale width

  for (int i = 0; i < N_PIXELS; i++) {
    // Create sine wave effect
    int sineVal = sin8((i * 255 / N_PIXELS) - (counter >> 8));

    // Map sine value to brightness
    byte brightnessVal = map(sineVal, 0, 255, 0, brightness);

    // Apply color with calculated brightness
    leds[i] = CHSV(myhue, 255, brightnessVal);
  }

  // Cycle through colors slowly
  EVERY_N_MILLISECONDS(500) {
    myhue++;
  }

  FastLED.show();
  vTaskDelay(pdMS_TO_TICKS(20));  // 50 FPS
}
