
// External parameters defined in main.cpp
extern uint8_t cometSpeed;
extern uint8_t cometTrail;
extern bool cometBlur;

class Comet {
public:
  Comet(){};
  void runPattern();
private:
};

void Comet::runPattern()
{
  // Original improved algorithm with configurable parameters
  static byte hue = HUE_RED;
  static int iDirection = 1;
  static double iPos = 0.0;
  static unsigned long lastUpdate = 0;

  // Map cometSpeed (0-255) to delay (30ms - 200ms)
  unsigned long delayMs = map(cometSpeed, 0, 255, 200, 30);
  unsigned long currentTime = millis();

  if (currentTime - lastUpdate >= delayMs) {
    lastUpdate = currentTime;

    // Calculate actual comet size based on cometTrail parameter
    int cometSize = map(cometTrail, 0, 10, 2, 8);
    const int deltaHue = 4;

    hue += deltaHue;
    iPos += iDirection * (cometSpeed / 16.0);

    if (iPos >= (N_PIXELS - cometSize) || iPos <= 0) {
      iDirection *= -1;
    }

    // Clear previous comet positions
    if (cometBlur) {
      // Blurred trail - slower fade
      fadeToBlackBy(leds, N_PIXELS, 30);
    } else {
      // Sharp trail - faster fade
      fadeToBlackBy(leds, N_PIXELS, 80);
    }

    // Draw comet head
    for (int i = 0; i < cometSize; i++) {
      int pos = (int)iPos + i;
      if (pos >= 0 && pos < N_PIXELS) {
        // Fade intensity along the trail
        uint8_t intensity = map(i, 0, cometSize - 1, 255, 50);
        leds[pos] = CHSV(hue, 255, brightness * intensity / 255);
      }
    }

    FastLED.show();
  }

  vTaskDelay(pdMS_TO_TICKS(10));
}
