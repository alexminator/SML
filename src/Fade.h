// WLED-based Fade effect - smooth fade between colors
// Adapted from WLED mode_fade implementation
class Fade {
  public:
    Fade(){};
    void runPattern();
  private:
};

void Fade::runPattern() {
  // WLED-compatible parameter
  extern uint8_t fadeSpeed;     // Default: 128, range: 0-255 (fade speed)

  static uint8_t currentHue = 0;
  static uint8_t targetHue = 0;
  static uint8_t fadeProgress = 0;

  // Calculate fade increment based on speed
  uint8_t fadeIncrement = map(fadeSpeed, 0, 255, 1, 10);

  // Update fade progress
  fadeProgress += fadeIncrement;

  if (fadeProgress >= 255) {
    // Fade complete, start new fade
    fadeProgress = 0;
    currentHue = targetHue;
    targetHue = random8();  // New random target color
  }

  // Interpolate between current and target hue
  uint8_t displayHue = map(fadeProgress, 0, 255, 0, 255);

  // Blend colors
  CRGB color1 = CHSV(currentHue, 255, brightness);
  CRGB color2 = CHSV(targetHue, 255, brightness);

  // Apply blended color to all LEDs
  for (int i = 0; i < N_PIXELS; i++) {
    leds[i] = color1.lerp8(color2, fadeProgress);
  }

  FastLED.show();
  vTaskDelay(pdMS_TO_TICKS(20));  // 50 FPS
}
