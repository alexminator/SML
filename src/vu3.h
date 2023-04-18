/*
 * VU: Rainbow from bottom or middle with hue cycling
 */

void vu6(bool is_centered, uint8_t channel) {

  const uint8_t SPEED = 10;
  static uint8_t hueOffset = 30;

  CRGB* vuleds;
  uint8_t i = 0;
  uint8_t *peak;      // Pointer variable declaration
  uint16_t height = auxReading(channel);

  if(channel == 0) {
    vuleds = leds;    // Store address of peakLeft in peak, then use *peak to
    peak = &peakLeft;   // access the value of that address
  }
  

  if(height > *peak)
    *peak = height; // Keep 'peak' dot at top
  
  EVERY_N_MILLISECONDS(SPEED) {hueOffset++;}

  if(is_centered) {
    // Color pixels based on rainbow gradient
    for (uint8_t i = 0; i < N_PIXELS_HALF; i++) {
      if (i >= height) {
        vuleds[N_PIXELS_HALF - i - 1] = CRGB::Black;
        vuleds[N_PIXELS_HALF + i] = CRGB::Black;
      } else {
        vuleds[N_PIXELS_HALF - i - 1] = CHSV(hueOffset + (10 * i),255,255);
        vuleds[N_PIXELS_HALF + i] = CHSV(hueOffset + (10 * i),255,255);
      }
    }

    // Draw peak dot  
    if (*peak > 0 && *peak <= N_PIXELS_HALF - 1) {
      vuleds[N_PIXELS_HALF - *peak - 1] = CHSV(hueOffset,255,255);
      vuleds[N_PIXELS_HALF + *peak] = CHSV(hueOffset,255,255);
    }
  }
  
  else {
    // Color pixels based on rainbow gradient
    for (uint8_t i = 0; i < N_PIXELS; i++) {
      if (i >= height) {
        vuleds[i] = CRGB::Black;
      } else {
        vuleds[i] = CHSV(hueOffset + (10 * i),255,255);
      }
    }
    // Draw peak dot  
    if (*peak > 0 && *peak <= N_PIXELS - 1)
      vuleds[*peak] = CHSV(hueOffset, 255, 255);
  
  }
  dropPeak(channel);

  averageReadings(channel);

  FastLED.show();
}
