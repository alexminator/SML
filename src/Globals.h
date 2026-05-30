// Globals.h
#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <FastLED.h>

// Effects
#define GRAVITY -1  // Downward (negative) acceleration of gravity in m/s^2
#define h0 1        // Starting height, in meters, of the ball (strip length)
#define NUM_BALLS 3 // Number of bouncing balls you want (recommend < 7, but 20 is fun in its own way)
#define FADE_RATE 2 // How long should the trails be. Very low value = longer trails.

// DECLARACIONES (con extern) - no ocupan memoria aquí
extern uint8_t volCount;
extern int vol[];
extern int lvl;

extern CRGBPalette16 currentPalette;
extern CRGBPalette16 targetPalette;

// Initial brightness (0-255)
// 130 = ~50% - comfortable starting point for indoor use
// Prevents eye strain from full brightness while still being visible
extern uint8_t myhue;
extern CRGB leds[];

// ============================================================================
// EFFECT PARAMETERS (configurable via web in future)
// ============================================================================

// Fire Effect Parameters
extern uint8_t fireCooling;   // 0-255, default 55 (cooling rate)
extern uint8_t fireSparking;  // 0-255, default 50 (sparking probability)
extern bool fireReverse;      // true = inverted fire

// Balls Effect Parameters
extern uint8_t ballsCount;          // 1-16 balls
extern bool ballsRandomColors;      // true = random, false = sequential

// Sinelon Effect Parameters
extern uint8_t sinelonBeat;  // 0-255, beat frequency
extern uint8_t sinelonFade;  // 0-255, fade rate

// Twinkle Effect Parameters (WLED TwinkleFOX algorithm)
extern uint8_t twinkleSpeed;       // 0-255, twinkle speed
extern uint8_t twinkleIntensity;   // 0-255, cooling/density
extern bool twinkleRedCool;        // true = shift to red when fading (like incandescent)

// Rainbow Beat Effect Parameters
extern uint8_t rainbowSpeed;     // 0-255, beat frequency
extern uint8_t rainbowDelta;     // 0-255, hue delta between LEDs

// Moving Dot Effect Parameters
extern uint8_t movingDotSpeed;   // 0-255, movement speed
extern uint8_t movingDotFade;    // 0-255, fade rate

// Ripple Effect Parameters
extern uint8_t rippleSize;     // 0-7, ripple wave size
extern bool rippleMirror;      // true = mirrored ripple (better than WLED's overlay)

// Comet Effect Parameters (original improved version)
extern uint8_t cometSpeed;      // 0-255, comet movement speed
extern uint8_t cometTrail;      // 0-10, trail length
extern bool cometBlur;          // true = blurred trail

// Breath Effect Parameters (WLED-compatible)
extern uint8_t breathSpeed;     // Default: 128, range: 0-255 (breathing speed)

// ColorSweep Effect Parameters (WLED-compatible)
extern uint8_t sweepSpeed;      // Default: 128, range: 0-255 (sweep speed)

// Juggle Effect Parameters
extern uint8_t juggleDots;       // 1-16, number of dots (better than WLED's 8 hardcoded)
extern uint8_t juggleSpeed;      // 0-255, movement frequency (WLED: speed)
extern uint8_t juggleIntensity;  // 0-255, fade rate/trail (WLED: intensity)

// balls effect
extern float h[];
extern float vImpact0;
extern float vImpact[];
extern float tCycle[];
extern int pos[];
extern long tLast[];
extern float COR[];

// VU
extern uint8_t volCountLeft;
extern int volLeft[];
extern int lvlLeft;
extern int minLvlAvgLeft;
extern int maxLvlAvgLeft;
extern bool is_centered;

// Declaración forward del struct
struct StripLed;

// Declaración del objeto externo
extern StripLed stripLed;

#endif // GLOBALS_H