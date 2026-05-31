#pragma once

#include <FastLED.h>

// ============================================================================
// UTILITY FUNCTIONS (extraídas de common.h)
// ============================================================================

/// Add glitter to LEDs (from Mark Kriegsman)
inline void addGlitter(fract8 chanceOfGlitter) {
    if (random8() < chanceOfGlitter) {
        leds[random16(N_PIXELS)] += CRGB::White;
    }
}

/// Input a value 0 to 255 to get a color value.
/// The colours are a transition r - g - b - back to r.
inline CRGB wheel(byte WheelPos) {
    if (WheelPos < 85) {
        return CRGB(WheelPos * 3, 255 - WheelPos * 3, 0);
    } else if (WheelPos < 205) {
        WheelPos -= 85;
        return CRGB(255 - WheelPos * 3, 0, WheelPos * 3);
    } else {
        WheelPos -= 205;
        return CRGB(0, WheelPos * 3, 255 - WheelPos * 3);
    }
}
