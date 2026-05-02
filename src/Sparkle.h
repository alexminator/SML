// WLED-based Sparkle effect - one LED sparkling at a time
// Adapted from WLED mode_sparkle implementation
class Sparkle {
  public:
    Sparkle(){};
    void runPattern();
  private:
};

void Sparkle::runPattern() {
  // WLED-compatible parameter
  extern uint8_t sparkleSpeed;  // Default: 128, range: 0-255 (sparkle speed)
  extern bool sparkleOverlay;   // Default: false (true = overlay on existing)

  static uint16_t sparkleIndex = 0;
  static unsigned long lastTime = 0;

  // If not overlay mode, fill with dim color
  if (!sparkleOverlay) {
    for (int i = 0; i < N_PIXELS; i++) {
      leds[i] = CHSV(myhue, 255, brightness >> 3);  // Very dim
    }
  }

  unsigned long currentTime = millis();
  uint32_t cycleTime = 10 + (255 - sparkleSpeed) * 2;

  if (currentTime - lastTime >= cycleTime) {
    lastTime = currentTime;

    // Pick random LED
    sparkleIndex = random16(N_PIXELS);

    // Make it sparkle at full brightness
    leds[sparkleIndex] = CHSV(myhue, 255, brightness);

    // Cycle through colors slowly
    EVERY_N_MILLISECONDS(1000) {
      myhue++;
    }
  }

  FastLED.show();
  vTaskDelay(pdMS_TO_TICKS(20));  // 50 FPS
}
