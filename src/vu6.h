/*
 * VU: Ocean waves (stereo)
 */

class OceanVU {
  public:
    OceanVU(){};
    void runPattern();
  private:
};

void sndwave() {
  int n = analogRead(AUDIO_IN_PIN) / 4 ;
  int sampleLeft = abs(n - BIAS - DC_OFFSET);
  //Serial.println("ADC read " + String(n));
  //Serial.println("VU 9 sample " + String(sampleLeft));
  leds[N_PIXELS_HALF] = ColorFromPalette(currentPalette, sampleLeft, sampleLeft * 2, LINEARBLEND); // Put the sample into the center
  
  for (int i = N_PIXELS - 1; i > N_PIXELS_HALF; i--) { //move to the left      // Copy to the left, and let the fade do the rest.
   leds[i] = leds[i - 1];
  }

  for (int i = 0; i < N_PIXELS_HALF; i++) { // move to the right    // Copy to the right, and let the fade do the rest.
    leds[i] = leds[i + 1];
  }
}

void OceanVU::runPattern() {
  currentPalette = PartyColors_p; // Initial palette.
  EVERY_N_SECONDS(5) { // Change the palette every 5 seconds.
    for (int i = 0; i < 16; i++) {
      targetPalette[i] = CHSV(random8(), 255, 255);
    }
  }

  EVERY_N_MILLISECONDS(100) { // Palette blending capability once they do change.
    uint8_t maxChanges = 24;
    nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);
  }

  EVERY_N_MILLIS_I(thistimer, 20) { // For fun, let's make the animation have a variable rate.
    uint8_t timeval = beatsin8(10, 20, 50); // Use a sinewave for the line below. Could also use peak/beat detection.
    thistimer.setPeriod(timeval); // Allows you to change how often this routine runs.
    fadeToBlackBy(leds, N_PIXELS, 16); // 1 = slow, 255 = fast fade. Depending on the faderate, the LED's further away will fade out.
    sndwave();
  }

  FastLED.setBrightness(255);
  FastLED.show();
}


