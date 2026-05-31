// ──────────────────────────────────────────────────────────────────────────────
// EffectRegistry.cpp — Single definition of effect table
// ──────────────────────────────────────────────────────────────────────────────
#include "../state/AppState.h"   // Must precede EffectRegistry.h (needs StripLed)
#include "EffectRegistry.h"

// ============================================================================
// EFFECT REGISTRY — definición única (ODR-safe)
// ============================================================================
// El orden debe coincidir con effectMap del frontend (data/js/main.js)
// y con los IDs históricos del switch original.
// ============================================================================

const EffectEntry effectRegistry[] = {
    { "fireStatus",             &StripLed::runFire },
    { "movingdotStatus",        &StripLed::runMovingDot },
    { "rainbowbeatStatus",      &StripLed::runRainbowBeat },
    { "rwbStatus",              &StripLed::runRedWhiteBlue },
    { "rippleStatus",           &StripLed::runRipple },
    { "twinkleStatus",          &StripLed::runTwinkle },
    { "ballsStatus",            &StripLed::runBalls },
    { "juggleStatus",           &StripLed::runJuggle },
    { "sinelonStatus",          &StripLed::runSinelon },
    { "cometStatus",            &StripLed::runComet },
    { "breathStatus",           &StripLed::runBreath },
    { "colorSweepStatus",       &StripLed::runColorSweep },
    { "rainbowVUStatus",        &StripLed::runRainbowVU },
    { "oldVUStatus",            &StripLed::runOldVU },
    { "rainbowHueVUStatus",     &StripLed::runRainbowHueVU },
    { "rippleVUStatus",         &StripLed::runRippleVU },
    { "threebarsVUStatus",      &StripLed::runThreebarsVU },
    { "oceanVUStatus",          &StripLed::runOceanVU },
    { "tempNEOStatus",          &StripLed::runTemperature },
    { "battNEOStatus",          &StripLed::runBattery },
};

constexpr uint8_t EFFECT_COUNT =
    sizeof(effectRegistry) / sizeof(effectRegistry[0]);
