class Charge {
public:
  Charge() {};
  void runPattern(int lvlCharge);
private:
   void setPixel(int pixel, byte red, byte green, byte blue); 
   // Add count parameter
   void setAll(byte red, byte green, byte blue, int count);
   void RunningLights(byte red, byte green, byte blue, int waveDelay, int count); 
   void meteorRain(byte red, byte green, byte blue, byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay, int SpeedDelay, int count);
};

void Charge::runPattern(int lvlCharge)
{
  FastLED.clear();
  int B = map(lvlCharge, 0, 100, 0, N_PIXELS);

  if ( B <= 7)
  {
    meteorRain(0xff, 0, 0, 10, 64, true, 20, B);      // red
  } else if ( B <= 14)
  {
    meteorRain(0xff, 0xff, 0, 10, 96, true, 30, B); // yellow
  } else if ( B < 24)
  {
    meteorRain(0, 0xff, 0, 10, 128, true, 45, B);   // green
  } else {
    RunningLights(0x00, 0x00, 0xff, 50, B);  // blue
  }
}

// Set a LED color (not yet visible)
void Charge::setPixel(int Pixel, byte red, byte green, byte blue) {
   leds[Pixel] = CRGB(red, green, blue);
}

void Charge::RunningLights(byte red, byte green, byte blue, int WaveDelay, int count) {
  int Position=0;
  for(int j=0; j < count * 2; j++)
  {
      for(int i = 0; i < count; i++) {
        float level = sin(i+j) * 127 + 128 / 255;
        setPixel(i, level * red, level * green, level * blue); // Set the LED color
      }
     
      FastLED.show();
      delay(WaveDelay);
  }
}

void fadeToBlack(int ledNo, byte fadeValue) {
   // FastLED
   leds[ledNo].fadeToBlackBy( fadeValue );
}

// Set all LEDs to a given color and apply it (visible)
void Charge::setAll(byte red, byte green, byte blue, int count) {
  for(int i = 0; i < count; i++ ) {
    Charge::setPixel(i, red, green, blue); 
  }
  FastLED.show();
}

void Charge::meteorRain(byte red, byte green, byte blue, byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay, int SpeedDelay, int count) {  

  Charge::setAll(0,0,0, count);
 
  for(int i = 0; i < count+count; i++) {
   
    // fade brightness all LEDs one step
    for(int j=0; j<count; j++) {
      if( (!meteorRandomDecay) || (random(10)>5) ) {
        fadeToBlack(j, meteorTrailDecay );        
      }
    }
   
    // draw meteor
    for(int j = 0; j < meteorSize; j++) {
      if( ( i-j <count) && (i-j>=0) ) {
        Charge::setPixel(i-j, red, green, blue);
      }
    }
   
    FastLED.show();
    vTaskDelay(pdMS_TO_TICKS(SpeedDelay));
  }
}




