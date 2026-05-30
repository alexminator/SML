#include "Globals.h"
class Twinkle {
  public:
    Twinkle(){};
    void runPattern();
  private:
};

// TwinkleFOX by Mark Kriegsman - WLED compatible implementation
// This effect creates smooth fade in/out twinkles like holiday lights
// Replaces WLED segments with fixed strip approach for ESP32
void Twinkle::runPattern() {
  static uint16_t PRNG16 = 11337;
  static unsigned long lastUpdate = 0;
  uint32_t ms = millis();

  // Calculate speed delay: maps speed 0-255 to delay 100-10ms
  // Speed 0 = 100ms (slow), Speed 255 = 10ms (fast)
  uint16_t speedDelay = map(twinkleSpeed, 0, 255, 100, 10);
  
  // Check if it's time to update
  if (ms - lastUpdate < speedDelay) {
    FastLED.show();  // Keep showing current state
    return;
  }
  lastUpdate = ms;

  // Calculate time in units suitable for smooth animation
  uint32_t ticks = ms / speedDelay;

  // Calculate twinkle density: intensity 0-255 maps to density 1-8
  // This determines how many pixels can twinkle at once
  uint8_t twinkleDensity = (twinkleIntensity >> 5) + 1;  // Range: 1-9

  // Fade out previous frame (creates trail effect)
  fadeToBlackBy(leds, N_PIXELS, 200);  // Fade by 200/256 ≈ 78%

  // Process each LED
  PRNG16 = 11337;
  for (unsigned i = 0; i < N_PIXELS; i++) {
    uint8_t salt = i * 3;

    // Generate pseudo-random values using improved PRNG
    uint16_t prng = PRNG16;
    prng = (prng * 2053) + 1384;
    
    // Calculate fast and slow cycle components
    uint8_t fastcycle8 = ticks & 0xFF;  // Cycles every 256 ticks
    uint16_t slowcycle16 = (ticks >> 8) + salt;
    slowcycle16 += sin8(slowcycle16);
    slowcycle16 = (slowcycle16 * 2053) + 1384;
    uint8_t slowcycle8 = (slowcycle16 & 0xFF) + (slowcycle16 >> 8);

    // Determine if this pixel should twinkle
    // Using only the bits that matter for density check
    if (((slowcycle8 & 0x0E) / 2) < twinkleDensity) {
      uint8_t bright;
      uint8_t ph = fastcycle8;

      // TwinkleFOX smooth fade pattern
      // Creates triangle wave that goes: 0 → 255 → 0 over ~256 ticks
      if (ph < 86) {
        // Attack phase: 0-86 ticks = 0-255 brightness (smooth ramp up)
        bright = ph * 3;  // 0 to 258, clamped to 255
      } else {
        // Decay phase: 86-256 ticks = 255-0 brightness (smooth ramp down)
        ph -= 86;  // Remaining ticks: 0-170
        bright = 255 - (ph + (ph / 2));  // Faster decay than attack
      }

      if (bright > 0) {
        // Calculate hue: varies per-pixel but consistent over time
        uint8_t hue = slowcycle8 - salt;

        // Create color with full saturation and brightness scaled
        // Brightness is scaled by global brightness setting
        uint8_t scaledBright = scale8(bright, stripLed.brightness);
        CRGB c = CHSV(hue, 255, scaledBright);

        // Optional: incandescent bulb effect
        // Shift color toward red/warm when fading
        if (twinkleRedCool && (fastcycle8 >= 128)) {
          // Cooling effect: reduce green and blue more than red
          uint8_t cooling = (fastcycle8 - 128) >> 4;  // 0-8 range
          c.g = qsub8(c.g, cooling);
          c.b = qsub8(c.b, cooling * 2);
        }

        leds[i] = c;
      }
    }

    // Advance PRNG for next pixel
    PRNG16 += salt;
  }

  FastLED.show();
}
