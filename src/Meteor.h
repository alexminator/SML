// WLED-based Meteor effect - smooth falling meteors with trails
// Adapted from WLED mode_meteor implementation
class Meteor {
  public:
    Meteor(){};
    void runPattern();
  private:
};

void Meteor::runPattern() {
  // WLED-compatible parameters
  extern uint8_t meteorSpeed;   // Default: 128, range: 0-255 (fall speed)
  extern uint8_t meteorTrail;   // Default: 4, range: 0-10 (trail length)
  extern bool meteorSmooth;     // Default: false (true = smooth/gradient trail)

  static double meteorPos = 0.0;
  static uint8_t meteorHue = 0;

  // Calculate actual speed and trail size
  double actualSpeed = (double)meteorSpeed / 32.0;  // 0-8 range
  int trailSize = map(meteorTrail, 0, 10, 2, 12);

  // Fade all LEDs
  fadeToBlackBy(leds, N_PIXELS, 200);

  // Update meteor position
  meteorPos += actualSpeed;
  if (meteorPos >= N_PIXELS + trailSize) {
    meteorPos = 0.0;
    meteorHue += 16;  // Cycle color on each new meteor
  }

  // Draw meteor head and trail
  for (int i = 0; i < trailSize; i++) {
    int ledPos = (int)meteorPos - i;

    if (ledPos >= 0 && ledPos < N_PIXELS) {
      if (meteorSmooth) {
        // Gradient trail - fade from head to tail
        byte brightnessVal = map(i, 0, trailSize - 1, 255, 50);
        leds[ledPos] = CHSV(meteorHue, 255, brightnessVal);
      } else {
        // Sharp trail - full brightness
        leds[ledPos] = CHSV(meteorHue, 255, brightness);
      }
    }
  }

  FastLED.show();
  vTaskDelay(pdMS_TO_TICKS(20));  // 50 FPS
}
