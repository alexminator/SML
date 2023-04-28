/*
 * VU: Rainbow from bottom or middle, green through purple
 */
class RainbowVU {
  public:
    RainbowVU(){};
    void runPattern(bool is_centered, uint8_t channel);
  private:
};

uint8_t rainbowHue2(uint8_t pixel, uint8_t num_pixels) {
  uint8_t hue = 96 - pixel * (145 / num_pixels);
  return hue;
}

void RainbowVU::runPattern(bool is_centered, uint8_t channel) {
  
  CRGB* vuleds;
  uint8_t i = 0;
  uint8_t *peak;      // Pointer variable declaration
  uint16_t height = auxReading(channel);
  Serial.println(height);
  if(channel == 0) {
    vuleds = leds;    // Store address of peakLeft in peak, then use *peak to
    peak = &peakLeft;   // access the value of that address
  }
  
  
  // Draw vu meter part
  fill_solid(vuleds, N_PIXELS, CRGB::Black);
  if(is_centered) {
    
    // Fill with colour gradient
    fill_gradient(vuleds, N_PIXELS_HALF  , CHSV(96, 255, 255), N_PIXELS - 1, CHSV(224, 255, 255),SHORTEST_HUES);
    fill_gradient(vuleds, N_PIXELS_HALF-1, CHSV(96, 255, 255), 0, CHSV(224, 255, 255),LONGEST_HUES);
    
    // Black out ends
    for (i = 0; i < N_PIXELS; i++) {
      uint8_t numBlack = (N_PIXELS - constrain(height, 0, N_PIXELS-1)) / 2;
      if(i <= numBlack -1 || i >= N_PIXELS - numBlack) leds[i] = CRGB::Black;
    }
    
    // Draw peak dot  
    if(height/2 > *peak)
      *peak = height/2; // Keep 'peak' dot at top

    if(*peak > 0 && *peak <= N_PIXELS_HALF-1) {
      vuleds[N_PIXELS_HALF + *peak]   = CHSV(rainbowHue2(*peak, N_PIXELS_HALF),255,255);
      vuleds[N_PIXELS_HALF - 1 - *peak] = CHSV(rainbowHue2(*peak, N_PIXELS_HALF),255,255);
    }
  } 
  
  else {
    // Fill with color gradient
    fill_gradient(leds, 0, CHSV(96, 255, 255), N_PIXELS - 1, CHSV(224, 255, 255),SHORTEST_HUES);
    
    //Black out end
    for (i = 0; i < N_PIXELS; i++) {
      if(i >= height) leds[i] = CRGB::Black;
    }
    
    // Draw peak dot  
    if(height > *peak)
      *peak = height; // Keep 'peak' dot at top
      
    if(*peak > 0 && *peak <= N_PIXELS-1)
      leds[*peak] = CHSV(rainbowHue2(*peak, N_PIXELS), 255, 255); // Set peak colour correctly
  }

  dropPeak(channel);
  averageReadings(channel);
  FastLED.show();
}


