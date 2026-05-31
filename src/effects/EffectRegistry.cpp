// ──────────────────────────────────────────────────────────────────────────────
// EffectRegistry.cpp — Single definition of effect table
// ──────────────────────────────────────────────────────────────────────────────
#include "../state/AppState.h"   // Must precede EffectRegistry.h (needs StripLed)
#include "EffectRegistry.h"

// Effect headers (all subclass Effect)
#include "../Fire.h"
#include "../MovingDot.h"
#include "../RainbowBeat.h"
#include "../RedWhiteBlue.h"
#include "../Ripple.h"
#include "../Twinkle.h"
#include "../Balls.h"
#include "../Juggle.h"
#include "../Sinelon.h"
#include "../Comet.h"
#include "../Breath.h"
#include "../ColorSweep.h"
#include "../Temp.h"
#include "../Battery.h"
#include "../vu1.h"
#include "../vu2.h"
#include "../vu3.h"
#include "../vu4.h"
#include "../vu5.h"
#include "../vu6.h"

// ============================================================================
// EFFECT REGISTRY — definición única (ODR-safe)
// ============================================================================
// El orden debe coincidir con effectMap del frontend (data/js/main.js)
// y con los IDs históricos del switch original.
// ============================================================================

const EffectEntry effectRegistry[] = {
    { "fireStatus",             new FireEffect(leds, N_PIXELS) },
    { "movingdotStatus",        new MovingDotEffect(leds, N_PIXELS) },
    { "rainbowbeatStatus",      new RainbowBeatEffect(leds, N_PIXELS) },
    { "rwbStatus",              new RedWhiteBlueEffect(leds, N_PIXELS) },
    { "rippleStatus",           new RippleEffect(leds, N_PIXELS) },
    { "twinkleStatus",          new TwinkleEffect(leds, N_PIXELS) },
    { "ballsStatus",            new BallsEffect(leds, N_PIXELS) },
    { "juggleStatus",           new JuggleEffect(leds, N_PIXELS) },
    { "sinelonStatus",          new SinelonEffect(leds, N_PIXELS) },
    { "cometStatus",            new CometEffect(leds, N_PIXELS) },
    { "breathStatus",           new BreathEffect(leds, N_PIXELS) },
    { "colorSweepStatus",       new ColorSweepEffect(leds, N_PIXELS) },
    { "rainbowVUStatus",        new RainbowVUEffect(leds, N_PIXELS) },
    { "oldVUStatus",            new OldskoolVUEffect(leds, N_PIXELS) },
    { "rainbowHueVUStatus",     new RainbowHueVUEffect(leds, N_PIXELS) },
    { "rippleVUStatus",         new RippleVUEffect(leds, N_PIXELS) },
    { "threebarsVUStatus",      new ThreebarsVUEffect(leds, N_PIXELS) },
    { "oceanVUStatus",          new OceanVUEffect(leds, N_PIXELS) },
    { "tempNEOStatus",          new TemperatureEffect(leds, N_PIXELS) },
    { "battNEOStatus",          new ChargeEffect(leds, N_PIXELS) },
};

constexpr uint8_t EFFECT_COUNT =
    sizeof(effectRegistry) / sizeof(effectRegistry[0]);
