// WLED-based Breath effect - "standby-breathing" of well known i-Devices
// Adapted from WLED mode_breath implementation
class Breath {
  public:
    Breath(){};
    void runPattern();
  private:
};

void Breath::runPattern() {
  // WLED-compatible parameter
  extern uint8_t breathSpeed;  // Default: 128, range: 0-255 (breathing speed)

  // WLED algorithm: time-based counter with speed
  unsigned long currentTime = millis();
  unsigned counter = (currentTime * ((breathSpeed >> 3) + 10)) & 0xFFFFU;

  // Calculate breath intensity using sine wave
  // Create smooth breathing effect: 0 -> 255 -> 0
  byte breathIntensity = sin8(counter >> 8);

  // Apply breath intensity to current color
  for (int i = 0; i < N_PIXELS; i++) {
    leds[i] = CHSV(myhue, 255, map(breathIntensity, 0, 255, 0, brightness));
  }

  // Cycle through colors slowly (optional)
  EVERY_N_MILLISECONDS(1000) {
    myhue++;
  }

  FastLED.show();
  vTaskDelay(pdMS_TO_TICKS(20));  // 50 FPS
}
