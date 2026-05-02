// WLED-based Dual Scan effect - two pixels moving in opposite directions
// Adapted from WLED mode_dual_scan implementation
class DualScan {
  public:
    DualScan(){};
    void runPattern();
  private:
};

void DualScan::runPattern() {
  // WLED-compatible parameter
  extern uint8_t scanSpeed;     // Default: 128, range: 0-255 (scan speed)
  extern bool scanOverlay;      // Default: false (true = overlay mode)

  int pos1, pos2;
  static uint16_t counter = 0;

  // Clear or overlay
  if (!scanOverlay) {
    fadeToBlackBy(leds, N_PIXELS, 200);
  }

  // Update position
  counter += (scanSpeed >> 3) + 1;

  // Use beatsin16 for smooth back-and-forth motion
  pos1 = beatsin16((scanSpeed >> 3) + 1, 0, N_PIXELS - 1);
  pos2 = N_PIXELS - 1 - pos1;  // Opposite direction

  // Set pixels at both positions
  leds[pos1] = CHSV(myhue, 255, brightness);
  leds[pos2] = CHSV(myhue + 128, 255, brightness);  // Complementary color

  // Cycle through colors slowly
  EVERY_N_MILLISECONDS(100) {
    myhue++;
  }

  FastLED.show();
  vTaskDelay(pdMS_TO_TICKS(20));  // 50 FPS
}
