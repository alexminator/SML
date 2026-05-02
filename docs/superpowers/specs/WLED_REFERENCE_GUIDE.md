# WLED Reference Guide for SML Project

**Date:** 2026-05-02  
**Purpose:** Comprehensive reference for implementing LED effects based on WLED architecture  
**WLED Project:** https://github.com/wled/WLED  
**WLED Source:** https://github.com/wled/WLED/tree/main/wled00

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Core Architecture](#2-core-architecture)
3. [Parameter System](#3-parameter-system)
4. [Effect Implementation Patterns](#4-effect-implementation-patterns)
5. [Top Effects Catalog](#5-top-effects-catalog)
6. [Audio-Reactive Effects](#6-audio-reactive-effects)
7. [FastLED Integration](#7-fastled-integration)
8. [Web Interface Architecture](#8-web-interface-architecture)
9. [Performance Optimization](#9-performance-optimization)
10. [Code Examples](#10-code-examples)
11. [Useful Functions](#11-useful-functions)
12. [Common Patterns](#12-common-patterns)
13. [Troubleshooting](#13-troubleshooting)
14. [Glossary](#14-glossary)

---

## 1. Project Overview

### 1.1 What is WLED?

**WLED** is an open-source firmware for ESP32/ESP8266 that controls LED strips with advanced effects:

- **220+ LED effects** with unified parameter system
- **Web interface** with real-time control via WebSocket
- **Audio-reactive effects** (WLED SR variant) with microphone input
- **Segment-based architecture** allowing multiple independent LED zones
- **Palette system** with 50+ color palettes
- **Active development:** 5+ years, 10K+ GitHub stars

### 1.2 Why WLED for SML?

**Advantages:**
- ✅ Proven stability (battle-tested by thousands of users)
- ✅ Optimized algorithms for ESP32 performance
- ✅ Unified parameter system (speed, intensity, custom1-3, check1-3)
- ✅ Comprehensive effect library (220+ effects)
- ✅ Active community and documentation
- ✅ Clean, maintainable code patterns

**Integration Strategy:**
- Adopt parameter system and effect structure
- Use proven algorithms from FX.cpp
- Maintain SML's power management and WiFi control
- Keep existing web UI architecture
- Replace simple effects with WLED versions gradually

### 1.3 Key WLED Files

**For Effect Implementation:**
- `wled00/FX.h` - Effect declarations and Segment class (55KB)
- `wled00/FX.cpp` - 220+ effect implementations (10,000+ lines)
- `wled00/const.h` - Constants and macros

**For Web Interface:**
- `wled00/data/index.htm` - Main HTML structure
- `wled00/data/index.js` - UI logic and effect controls
- `wled00/data/common.js` - WebSocket communication

---

## 2. Core Architecture

### 2.1 WS2812FX Base Class

WLED uses **WS2812FX** as the foundation for LED control:

```cpp
class WS2812FX {
  private:
    WS2812* leds;
    uint16_t numLeds;
    Segment* segments;
    uint8_t numSegments;
    
  public:
    void service();  // Main loop handler
    void setSegment(uint8_t n, uint16_t start, uint16_t stop);
    void setBrightness(uint8_t b);
    void setColor(uint8_t r, uint8_t g, uint8_t b);
};
```

**Key Features:**
- Manages LED strip state
- Handles segment allocation
- Calls effect render functions
- Automatic brightness control
- Palette integration

### 2.2 Segment Architecture

**Segment** is WLED's core abstraction for independent LED zones:

```cpp
struct Segment {
  uint16_t start;       // Start LED index
  uint16_t stop;        // Stop LED index
  uint8_t speed;        // Effect speed (0-255)
  uint8_t intensity;    // Effect intensity (0-255)
  uint8_t custom1;      // Custom parameter 1 (0-255)
  uint8_t custom2;      // Custom parameter 2 (0-255)
  uint8_t custom3;      // Custom parameter 3 (0-31)
  bool check1;          // Boolean option 1
  bool check2;          // Boolean option 2
  bool check3;          // Boolean option 3
  uint8_t mode;         // Current effect ID
  uint8_t palette;      // Color palette ID
  
  void runEffect();     // Render current effect
};
```

**Benefits:**
- Multiple independent effects on same strip
- Per-effect parameter control
- Easy to add/remove effects
- Memory efficient (shared LED array)

### 2.3 Effect Function Signature

**WLED effect pattern:**

```cpp
uint16_t mode_name(void) {
  // Effect implementation here
  // Returns: speed (delay in ms for next frame)
  return FRAMETIME;
}
```

**SML adaptation pattern:**

```cpp
class EffectName {
  private:
    EffectParams params;
    CRGB* leds;
    uint16_t numLeds;
    
  public:
    EffectName(CRGB* l, uint16_t n);
    void run();
    void setSpeed(uint8_t s);
    void setIntensity(uint8_t i);
    // ... other parameter setters
};
```

---

## 3. Parameter System

### 3.1 Standard Parameters

All WLED effects use this unified parameter system:

| Parameter | Range | Purpose | Common Usage |
|-----------|-------|---------|--------------|
| **speed** | 0-255 | Animation speed | Movement frequency, beat rate |
| **intensity** | 0-255 | Effect magnitude | Fade rate, number of objects, size |
| **custom1** | 0-255 | Effect-specific | Hue shift, color variation |
| **custom2** | 0-255 | Effect-specific | Secondary parameter |
| **custom3** | 0-31 | Effect-specific | Tertiary (limited range) |
| **check1** | boolean | Toggle option | Reverse mode, mirror |
| **check2** | boolean | Toggle option | Random colors, background |
| **check3** | boolean | Toggle option | Advanced options |

### 3.2 Default Values

```cpp
#define DEFAULT_SPEED      128  // Mid-range speed
#define DEFAULT_INTENSITY  128  // Mid-range intensity
#define DEFAULT_C1         128  // Mid-range custom1
#define DEFAULT_C2         128  // Mid-range custom2
#define DEFAULT_C3         16   // Mid-range custom3 (0-31)
```

### 3.3 Parameter Mapping Examples

**Fire 2012:**
```cpp
speed:     0-255 → sparking probability (255-s = 0-255)
intensity: 0-255 → cooling rate (map to 0-250)
check1:    reverse mode (fire falls from sky)
```

**Sinelon:**
```cpp
speed:     0-255 → beatsin16 frequency
intensity: 0-255 → fade rate (8-224)
```

**Bouncing Balls:**
```cpp
speed:     0-255 → bounce height
intensity: 0-255 → number of balls (map to 1-16)
check1:    random vs sequential colors
```

### 3.4 Accessing Parameters in Effects

```cpp
void render() {
  // Speed controls frequency
  uint8_t beat = beatsin16(255 - params.speed, 0, 255);
  
  // Intensity controls fade rate
  uint8_t fade = map(params.intensity, 0, 255, 8, 224);
  fadeToBlackBy(leds, numLeds, fade);
  
  // Custom1 controls hue shift
  uint8_t hue = baseHue + params.custom1;
  
  // Check1 for mode toggle
  if (params.check1) {
    // Reverse mode
  }
}
```

---

## 4. Effect Implementation Patterns

### 4.1 Basic Effect Template

```cpp
// EffectName.h
#pragma once
#include "FastLED.h"
#include "Effect.h"

class EffectName : public Effect {
  public:
    EffectName(CRGB* l, uint16_t n) : Effect(l, n) {}
    
  protected:
    void render() override {
      // Clear previous frame (optional)
      // fadeToBlackBy(leds, numLeds, FADE_RATE);
      
      // Update animation
      EVERY_N_MILLIS(FRAMETIME) {
        // Effect logic here
      }
      
      // Show LEDs
      FastLED.show();
    }
};
```

### 4.2 Pattern: Movement with beatsin16

```cpp
// Sinelon-like movement
void render() {
  // Calculate position based on speed
  uint8_t pos = beatsin16(255 - params.speed, 0, numLeds - 1);
  
  // Fade based on intensity
  uint8_t fade = map(params.intensity, 0, 255, 8, 224);
  fadeToBlackBy(leds, numLeds, fade);
  
  // Set LED at position
  leds[pos] = CHSV(hue, 255, brightness);
  
  FastLED.show();
}
```

### 4.3 Pattern: Fire 2012

```cpp
void render() {
  // Map parameters
  uint8_t cooling = map(params.intensity, 0, 255, 0, 250);
  uint8_t sparking = map(255 - params.speed, 0, 255, 0, 255);
  
  // Cool down
  for (int i = 0; i < numLeds; i++) {
    heat[i] = qsub8(heat[i], random8(0, (cooling * 10) / numLeds + 2));
  }
  
  // Spark
  if (random8() < sparking) {
    int y = random8(7);
    heat[y] = qadd8(heat[y], random8(160, 255));
  }
  
  // Map heat to colors
  for (int i = 0; i < numLeds; i++) {
    uint8_t colorIndex = scale8(heat[i], 240);
    CRGB color = ColorFromPalette(palette, colorIndex);
    leds[i] = color;
  }
  
  FastLED.show();
}
```

### 4.4 Pattern: Ripple Effect

```cpp
void render() {
  static int rippleCenter = 0;
  static int rippleStep = -1;
  
  // Start new ripple
  if (rippleStep == -1) {
    rippleCenter = random(numLeds);
    rippleStep = 0;
  }
  
  // Expand ripple
  if (rippleStep < MAX_STEPS) {
    int pos1 = (rippleCenter + rippleStep) % numLeds;
    int pos2 = (rippleCenter - rippleStep + numLeds) % numLeds;
    
    uint8_t brightness = pow(0.8, rippleStep) * 255;
    leds[pos1] = CHSV(hue, 255, brightness);
    leds[pos2] = CHSV(hue, 255, brightness);
    
    rippleStep++;
  } else {
    rippleStep = -1;  // Reset
  }
  
  FastLED.show();
}
```

---

## 5. Top Effects Catalog

### 5.1 Top 20 Non-Audio Effects (Most Popular)

Based on WLED usage statistics and visual appeal:

| Rank | Effect Name | Category | Description | Complexity |
|------|-------------|----------|-------------|------------|
| 1 | **Rainbow** | Color | Smooth rainbow gradient | Low |
| 2 | **Fire 2012** | Simulation | Realistic fire with cooling/sparking | Medium |
| 3 | **Sinelon** | Movement | Single dot moving with beat | Low |
| 4 | **Bouncing Balls** | Physics | Realistic bouncing balls with gravity | Medium |
| 5 | **Twinklefox** | Sparkle | Random twinkling with fade | Medium |
| 6 | **Ripple** | Wave | Expanding circular ripples | Medium |
| 7 | **Comet** | Movement | Comet with trail effect | Low |
| 8 | **Juggle** | Movement | Multiple colored dots | Low |
| 9 | **Chase** | Movement | Classic chase pattern | Low |
| 10 | **Plasma** | Wave | Smooth plasma waves | Medium |
| 11 | **Noise** | Pattern | Perlin noise effects | High |
| 12 | **Scanner** | Movement | Scanning dot with fade | Low |
| 13 | **Popcorn** | Simulation | Bouncing popcorn | Medium |
| 14 | **Sparkle** | Sparkle | Random white sparkles | Low |
| 15 | **Sunrise** | Transition | Sunrise simulation | Medium |
| 16 | **Pattern** | Pattern | Custom pattern effects | Medium |
| 17 | **Candle** | Simulation | Flickering candle | Low |
| 18 | **Flow** | Movement | Smooth flow effect | Low |
| 19 | **Gradient** | Color | Smooth color gradient | Low |
| 20 | **Fill** | Utility | Fill with solid color | Low |

### 5.2 Effects Already in SML (Mapped to WLED)

| SML Effect | SML ID | WLED Equivalent | WLED Features |
|------------|--------|-----------------|---------------|
| Moving Dot | 0 | **Scanner** | Trail, configurable speed |
| Rainbow Beat | 1 | **Rainbow** | Full rotation, speed control |
| Ripple | 3 | **Ripple** | Mirror mode, size control |
| Fire | 4 | **Fire 2012** | Cooling, sparking, reverse |
| Twinkle | 5 | **Twinklefox** | Density, background, sparkles |
| Bouncing Balls | 6 | **Bouncing Balls** | Ball count, height, colors |
| Juggle | 7 | **Juggle** | Dot count, speed |
| Sinelon | 8 | **Sinelon** | Speed, fade rate |
| Comet | 9 | **Comet** | Trail length, blur |
| **Total** | **9** | **9 effects** | All have parameter controls |

---

## 6. Audio-Reactive Effects

### 6.1 WLED SR Audio Effects

WLED SR variant includes **20+ audio-reactive effects** powered by microphone input:

| Rank | Effect | Description | Features |
|------|--------|-------------|----------|
| 1 | **Gradient** | Color gradient to music | Bass response |
| 2 | **Center Out** | Bars from center outward | Symmetrical |
| 3 | **Down the Hole** | Bars falling down | Perspective effect |
| 4 | **Noise Move** | Noise with audio | Perlin + audio |
| 5 | **Pixel** | Pixelated VU meter | Grid effect |
| 6 | **Solar Eclipse** | Pulsing circles | Bass hits |
| 7 | **Waves** | Wave patterns | Frequency based |
| 8 | **Juggles** | Moving dots with audio | Speed syncs |
| 9 | **Ripples** | Audio ripples | Expanding rings |
| 10 | **Dancing Shifts** | Color shifts to music | Palette based |

**Note:** SML already has VU effects (vu1-vu6) which are similar in concept but custom-implemented.

### 6.2 Audio Processing Architecture

**WLED SR audio flow:**

```
Microphone → ADC → FFT → Frequency Bands → Effect Parameters
                                   ↓
                            Bass/Mid/Treble
                                   ↓
                            Color/Brightness/Speed
```

**Key components:**
- **FFT (Fast Fourier Transform):** Converts audio to frequency spectrum
- **Frequency bands:** 16 bands (bass to treble)
- **Aggregation:** Bass, mid, treble extracted
- **Smoothing:** Exponential moving average prevents flicker

**SML Equivalent:**
- SML uses simple analog audio reading (ADC on pin D36)
- BIAS adjustment for DC offset
- NOISE threshold for filtering
- VU effects compute volume directly (no FFT)

---

## 7. FastLED Integration

### 7.1 Color Palettes

WLED integrates **50+ FastLED palettes**:

```cpp
// Common palettes
0:  RainbowColors
1:  RainbowStripeColors
2:  OceanColors
3:  CloudColors
4:  ForestColors
5:  PartyColors
6:  HeatColors
7:  LavaColors
8:  IceColors
... (50+ total)
```

**Using palettes in effects:**

```cpp
CRGBPalette16 currentPalette = PartyColors_p;
CRGB color = ColorFromPalette(currentPalette, hue, brightness, LINEARBLEND);
leds[pos] = color;
```

### 7.2 Blend Modes

FastLED provides blend modes for smooth color transitions:

```cpp
// LINEARBLEND: Smooth interpolation
leds[i] = ColorFromPalette(palette, hue, 255, LINEARBLEND);

// NOBLEND: Direct color selection
leds[i] = ColorFromPalette(palette, hue, 255, NOBLEND);
```

### 7.3 FastLED Utility Functions

**Essential functions for effects:**

```cpp
// Fade all LEDs
fadeToBlackBy(leds, numLeds, fadeAmount);  // 0-255
nscale8(leds, numLeds, scale);             // Dim by factor

// Color math
qadd8(a, b);    // Add with saturation at 255
qsub8(a, b);    // Subtract with floor at 0
scale8(value, scale);  // Scale 0-255 value

// Random
random8();                  // 0-255
random8(max);               // 0-max-1
random8(min, max);          // min-max-1

// Timing
EVERY_N_MILLIS(interval) {  // Execute every N ms
  // Code here
}

beatsin16(beat, min, max);  // Sine wave at BPM/16
```

---

## 8. Web Interface Architecture

### 8.1 WLED Web Structure

**HTML** (`index.htm`):
```html
<div id="picker"></div>                    <!-- Color picker -->
<select id="effect"></select>              <!-- Effect selector -->
<input type="range" id="speed">           <!-- Speed control -->
<input type="range" id="intensity">       <!-- Intensity control -->
<canvas id="liveview"></canvas>           <!-- Live preview -->
```

**JavaScript** (`index.js`):
```javascript
// Load effects dynamically
function populateEffects() {
  // Populate effect dropdown from JSON
}

// Update effect parameters
function updateEffect() {
  const data = {
    effect: currentEffect,
    speed: document.getElementById('speed').value,
    intensity: document.getElementById('intensity').value
  };
  ws.send(JSON.stringify(data));
}

// WebSocket message handler
ws.onmessage = (event) => {
  const state = JSON.parse(event.data);
  updateUI(state);
};
```

**WebSocket Protocol** (`common.js`):
```javascript
// Send effect change
function setEffect(id, speed, intensity) {
  const msg = {
    effect: id,
    speed: speed,
    intensity: intensity
  };
  ws.send(JSON.stringify(msg));
}

// Receive LED state
ws.onmessage = (event) => {
  const data = JSON.parse(event.data);
  // Update UI
};
```

### 8.2 SML Web Interface Compatibility

**Current SML web structure:**
- `data/index.html` - Main UI (similar structure to WLED)
- `data/js/main.js` - Effect control (already has WebSocket)
- **Compatible with WLED-style parameters** ✅

**Integration strategy:**
- Keep existing HTML structure
- Add parameter sliders (speed, intensity)
- Update WebSocket to send parameters
- No major HTML/JS rewrite needed

---

## 9. Performance Optimization

### 9.1 WLED Performance Techniques

**Frame Rate Control:**
```cpp
// Target: 30-60 FPS
#define FRAMETIME 16  // ~60 FPS
return FRAMETIME;  // Effect function return
```

**Memory Optimization:**
```cpp
// Reuse arrays instead of allocating
static uint8_t heat[MAX_LEDS];  // Persistent across calls

// Limit buffer sizes
#define MAX_BALLS 16
static ball balls[MAX_BALLS];
```

**CPU Optimization:**
```cpp
// Skip unnecessary rendering
if (!segment.isActive()) return;

// Use fixed-point math instead of float
uint8_t scaled = scale8(value, 255);  // Fast

// Batch LED operations
memset(leds, 0, sizeof(CRGB) * numLeds);  // Clear fast
```

### 9.2 Power Optimization (SML-Specific)

**For Battery Mode:**
- Reduce frame rate (30 FPS → 20 FPS)
- Lower CPU frequency (240MHz → 80MHz)
- Disable WiFi when not needed
- Use fadeToBlackBy instead of full redraw

**WLED doesn't have this** (mains-powered only), but SML can integrate:

```cpp
void render() {
  // Adjust frame rate based on power mode
  uint16_t frameTime = (powerMode == BATTERY_SLEEP) ? 50 : 16;
  
  EVERY_N_MILLIS(frameTime) {
    // Effect logic
  }
}
```

---

## 10. Code Examples

### 10.1 Fire 2012 Complete Implementation

```cpp
// Fire2012.h
#pragma once
#include "FastLED.h"
#include "Effect.h"

class Fire2012 : public Effect {
  private:
    uint8_t heat[N_PIXELS];
    CRGBPalette16 palette = HeatColors_p;
    
  public:
    Fire2012(CRGB* l, uint16_t n) : Effect(l, n) {
      memset(heat, 0, sizeof(heat));
    }
    
  protected:
    void render() override {
      // Map parameters
      uint8_t cooling = map(params.intensity, 0, 255, 0, 250);
      uint8_t sparking = map(255 - params.speed, 0, 255, 0, 255);
      
      // Cool down
      for (int i = 0; i < numLeds; i++) {
        heat[i] = qsub8(heat[i], random8(0, (cooling * 10) / numLeds + 2));
      }
      
      // Spark
      if (random8() < sparking) {
        int y = random8(7);
        heat[y] = qadd8(heat[y], random8(160, 255));
      }
      
      // Map heat to colors
      for (int i = 0; i < numLeds; i++) {
        uint8_t colorIndex = scale8(heat[i], 240);
        leds[i] = ColorFromPalette(palette, colorIndex);
      }
      
      FastLED.show();
    }
};
```

### 10.2 Sinelon Complete Implementation

```cpp
// Sinelon.h
#pragma once
#include "FastLED.h"
#include "Effect.h"

class Sinelon : public Effect {
  private:
    CRGBPalette16 palette = PartyColors_p;
    uint8_t hue = 0;
    
  public:
    Sinelon(CRGB* l, uint16_t n) : Effect(l, n) {}
    
  protected:
    void render() override {
      // Calculate position
      uint8_t pos = beatsin16(255 - params.speed, 0, numLeds - 1);
      
      // Fade based on intensity
      uint8_t fade = map(params.intensity, 0, 255, 8, 224);
      fadeToBlackBy(leds, numLeds, fade);
      
      // Set LED at position
      leds[pos] = ColorFromPalette(palette, hue, 255, LINEARBLEND);
      
      // Increment hue
      EVERY_N_MILLIS(20) {
        hue++;
      }
      
      FastLED.show();
    }
};
```

### 10.3 Bouncing Balls Complete Implementation

```cpp
// BouncingBalls.h
#pragma once
#include "FastLED.h"
#include "Effect.h"

class BouncingBalls : public Effect {
  private:
    struct Ball {
      float height;
      float velocity;
      float impactVelocity;
      CRGB color;
    };
    
    static const uint8_t GRAVITY = -9.81;
    static const float BOUNCE_DAMPING = 0.7;
    
    Ball balls[16];
    uint8_t numBalls = 4;
    
  public:
    BouncingBalls(CRGB* l, uint16_t n) : Effect(l, n) {
      numBalls = map(params.intensity, 0, 255, 1, 16);
      initBalls();
    }
    
  private:
    void initBalls() {
      for (uint8_t i = 0; i < numBalls; i++) {
        balls[i].height = numLeds - 1;
        balls[i].velocity = 0;
        balls[i].impactVelocity = 0;
        
        if (params.check1) {
          balls[i].color = CHSV(random8(), 255, 255);
        } else {
          balls[i].color = CHSV(i * 256 / numBalls, 255, 255);
        }
      }
    }
    
  protected:
    void render() override {
      fadeToBlackBy(leds, numLeds, 20);
      
      for (uint8_t i = 0; i < numBalls; i++) {
        float timeSinceLastFrame = 0.016;  // ~60 FPS
        
        // Update position
        balls[i].velocity += GRAVITY * timeSinceLastFrame;
        balls[i].height += balls[i].velocity * timeSinceLastFrame;
        
        // Bounce
        if (balls[i].height < 0) {
          balls[i].height = 0;
          balls[i].velocity = -balls[i].velocity * BOUNCE_DAMPING;
          
          // Stop if too slow
          if (abs(balls[i].velocity) < 0.5) {
            balls[i].velocity = 0;
          }
        }
        
        // Map height to LED
        uint8_t ledPos = constrain(balls[i].height, 0, numLeds - 1);
        leds[ledPos] = balls[i].color;
      }
      
      FastLED.show();
    }
};
```

---

## 11. Useful Functions

### 11.1 Timing Functions

```cpp
// Sine wave at BPM/16 rate
uint8_t pos = beatsin16(60, 0, numLeds - 1);  // 60 BPM

// Execute every N milliseconds
EVERY_N_MILLIS(50) {
  // Code here runs every 50ms
}

// Millisecond timer
unsigned long now = millis();
if (now - lastUpdate > interval) {
  lastUpdate = now;
  // Update code
}
```

### 11.2 Math Functions

```cpp
// Constrain value to range
uint8_t val = constrain(x, 0, 255);

// Map value from range to range
uint8_t newVal = map(value, 0, 255, 8, 224);

// Scale 8-bit value (faster than map)
uint8_t scaled = scale8(value, 255);

// Add/subtract with saturation
uint8_t sum = qadd8(a, b);    // Saturates at 255
uint8_t diff = qsub8(a, b);   // Floors at 0

// Random
uint8_t r = random8();           // 0-255
uint8_t r = random8(max);        // 0-max-1
uint8_t r = random8(min, max);   // min-max-1
```

### 11.3 LED Operations

```cpp
// Fade all LEDs
fadeToBlackBy(leds, numLeds, 50);  // Fade by 50/255
nscale8(leds, numLeds, 200);       // Dim to 200/255

// Clear all LEDs
fill_solid(leds, numLeds, CRGB::Black);

// Fill with color
fill_solid(leds, numLeds, CRGB::Red);

// Fade specific LED
leds[i].fadeToBlackBy(50);

// Blend colors
leds[i] = leds[i].lerp8(color, 128);  // 50% blend
```

### 11.4 Color Operations

```cpp
// From palette
CRGB color = ColorFromPalette(palette, hue, 255, LINEARBLEND);

// HSV color
CHSV hsv(hue, sat, val);
CRGB rgb = hsv;  // Convert to RGB

// RGB color
CRGB color(r, g, b);

// Color math
CRGB blended = color1 + color2;   // Additive
CRGB dimmed = color.nscale8(128); // Dim 50%
```

---

## 12. Common Patterns

### 12.1 Fade Then Draw

**Most effects use this pattern:**

```cpp
void render() {
  // 1. Fade previous frame
  fadeToBlackBy(leds, numLeds, fadeAmount);
  
  // 2. Calculate new positions
  uint8_t pos = beatsin16(speed, 0, numLeds - 1);
  
  // 3. Draw new state
  leds[pos] = color;
  
  // 4. Display
  FastLED.show();
}
```

### 12.2 Palette Rotation

**Cycle through palette over time:**

```cpp
static uint8_t paletteIndex = 0;

void render() {
  EVERY_N_MILLIS(50) {
    paletteIndex++;
  }
  
  uint8_t hue = paletteIndex;
  for (int i = 0; i < numLeds; i++) {
    leds[i] = ColorFromPalette(palette, hue + i);
  }
  
  FastLED.show();
}
```

### 12.3 Independent Objects

**Multiple objects with own state:**

```cpp
struct Dot {
  uint8_t pos;
  uint8_t hue;
  int8_t dir;
};

Dot dots[4];

void render() {
  fadeToBlackBy(leds, numLeds, 50);
  
  for (int i = 0; i < 4; i++) {
    // Update position
    dots[i].pos += dots[i].dir;
    
    // Bounce
    if (dots[i].pos == 0 || dots[i].pos == numLeds - 1) {
      dots[i].dir *= -1;
    }
    
    // Draw
    leds[dots[i].pos] = CHSV(dots[i].hue, 255, 255);
  }
  
  FastLED.show();
}
```

### 12.4 Wave Pattern

```cpp
void render() {
  for (int i = 0; i < numLeds; i++) {
    uint8_t value = sin8(i * 16 + millis() / 10);
    leds[i] = CHSV(hue, 255, value);
  }
  
  FastLED.show();
}
```

---

## 13. Troubleshooting

### 13.1 Effect Too Slow

**Symptoms:** Low frame rate, laggy animation

**Solutions:**
- Reduce LED count in calculations
- Use fixed-point math instead of float
- Decrease frame rate (30 FPS instead of 60)
- Optimize inner loops
- Use lookup tables instead of calculations

### 13.2 Colors Look Wrong

**Symptoms:** Incorrect colors, color artifacts

**Solutions:**
- Check COLOR_ORDER (GRB for WS2812B)
- Verify power supply can handle current
- Use LINEARBLEND for smooth gradients
- Check for color saturation (use qadd8)

### 13.3 Flickering

**Symptoms:** Random flicker in effects

**Solutions:**
- Use fadeToBlackBy instead of clear
- Add smoothing to random values
- Use static variables for state
- Check timing (consistent frame rate)

### 13.4 Memory Issues

**Symptoms:** Crashes, reboots

**Solutions:**
- Limit array sizes (MAX_LEDS, MAX_BALLS)
- Use `static` for persistent arrays
- Free unused objects
- Check heap with `ESP.getFreeHeap()`

### 13.5 SML-Specific Issues

**Power management interference:**
```cpp
// Check current power mode before heavy effects
if (powerMode == BATTERY_SLEEP) {
  // Use simpler effect or reduce frame rate
  return;
}
```

**WiFi state affects ADC:**
```cpp
// Don't use ADC2 pins when WiFi is active
// Use D36 (VP) or D39 (VN) - ADC1 only
```

---

## 14. Glossary

**BeatSin16**: FastLED function for sine wave movement at BPM/16 rate

**Cooling**: Fire effect parameter controlling heat decay rate

**Fade Rate**: How quickly LEDs dim between frames (8-224 typical)

**Frame Time**: Target milliseconds per frame (16ms = ~60 FPS)

**Heat Array**: Temperature values for fire simulation (0-255)

**Impact Velocity**: Speed of ball when hitting ground (bouncing)

**LINEARBLEND**: Smooth color interpolation between palette entries

**Palette**: Predefined color gradient (Rainbow, Ocean, Lava, etc.)

**QAdd8/QSub8**: Saturating arithmetic (no overflow/underflow)

**Scale8**: Fast 8-bit scaling (optimized multiplication)

**Sparkling**: Fire effect parameter for ignition probability

**Segment**: WLED abstraction for independent LED zones

**WS2812FX**: Base class for WLED LED control

---

## 15. Additional Resources

### 15.1 Official WLED Resources

- **GitHub:** https://github.com/wled/WLED
- **Wiki:** https://github.com/wled/WLED/wiki
- **Forum:** https://wled.discourse.group/
- **User Extensions:** https://github.com/wled-WLED-Usermods

### 15.2 FastLED Resources

- **FastLED GitHub:** https://github.com/FastLED/FastLED
- **FastLED Documentation:** https://fastled.io/docs/
- **Palette Reference:** https://fastled.io/docs/palette.html

### 15.3 SML Integration Notes

**Key differences SML vs WLED:**
- ✅ SML has power management (WLED doesn't)
- ✅ SML has battery backup (WLED doesn't)
- ✅ SML has Bluetooth control (WLED doesn't)
- ✅ SML has DHT sensor (WLED doesn't)
- ✅ SML already has web UI (compatible with WLED parameters)

**Adaptation strategy:**
- Keep SML's power management intact
- Adopt WLED's effect architecture
- Maintain SML's web UI structure
- Add parameter controls to existing web UI

---

## 16. Quick Reference

### 16.1 Parameter Mapping (Cheat Sheet)

| Effect | Speed | Intensity | Custom1 | Check1 |
|--------|-------|-----------|---------|--------|
| Fire 2012 | Sparking | Cooling | - | Reverse |
| Sinelon | Beat freq | Fade rate | - | - |
| Bouncing Balls | Height | Ball count | - | Random |
| Ripple | Expansion | Wave size | - | Mirror |
| Twinklefox | Fade rate | Density | - | Black BG |
| Juggle | Frequency | Dot count | - | - |
| Comet | Speed | Trail length | - | Blur |
| Rainbow | Rotation speed | Delta hue | - | - |

### 16.2 Common Code Snippets

**Beatsin movement:**
```cpp
uint8_t pos = beatsin16(255 - params.speed, 0, numLeds - 1);
```

**Fade to black:**
```cpp
fadeToBlackBy(leds, numLeds, map(params.intensity, 0, 255, 8, 224));
```

**Palette color:**
```cpp
leds[i] = ColorFromPalette(palette, hue, 255, LINEARBLEND);
```

**Random with timing:**
```cpp
EVERY_N_MILLIS(50) {
  // Code here
}
```

---

**End of WLED Reference Guide**

*This document is a comprehensive reference for implementing WLED-style LED effects in the SML project. Always refer to the official WLED repository for the latest code and updates.*