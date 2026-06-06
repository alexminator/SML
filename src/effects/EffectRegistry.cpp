// ──────────────────────────────────────────────────────────────────────────────
// EffectRegistry.cpp — Single definition of effect table
// ──────────────────────────────────────────────────────────────────────────────
#include "../state/AppState.h"   // Must precede EffectRegistry.h (needs StripLed)
#include "EffectRegistry.h"

#include <LittleFS.h>
#include <ArduinoJson.h>

// Effect headers (all subclass Effect)
#include "Fire.h"
#include "MovingDot.h"
#include "RainbowBeat.h"
#include "RedWhiteBlue.h"
#include "Ripple.h"
#include "Twinkle.h"
#include "Balls.h"
#include "Juggle.h"
#include "Sinelon.h"
#include "Comet.h"
#include "Breath.h"
#include "ColorSweep.h"

// ── Nuevos efectos WLED ──
#include "ColorWipe.h"
#include "TheaterChase.h"
#include "RunningLights.h"
#include "Dissolve.h"
#include "DualScan.h"
#include "Fade.h"
#include "Meteor.h"
#include "Sparkle.h"
#include "Fire2012.h"

#include "Temp.h"
#include "Battery.h"
#include "../vu/vu1.h"
#include "../vu/vu2.h"
#include "../vu/vu3.h"
#include "../vu/vu4.h"
#include "../vu/vu5.h"
#include "../vu/vu6.h"

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

    // ── Nuevos efectos WLED (21-29) ────────────────────────────────────────
    { "colorWipeStatus",        new ColorWipeEffect(leds, N_PIXELS) },
    { "theaterChaseStatus",     new TheaterChaseEffect(leds, N_PIXELS) },
    { "runningLightsStatus",    new RunningLightsEffect(leds, N_PIXELS) },
    { "dissolveStatus",         new DissolveEffect(leds, N_PIXELS) },
    { "dualScanStatus",         new DualScanEffect(leds, N_PIXELS) },
    { "fadeStatus",             new FadeEffect(leds, N_PIXELS) },
    { "meteorStatus",           new MeteorEffect(leds, N_PIXELS) },
    { "sparkleStatus",          new SparkleEffect(leds, N_PIXELS) },
    { "fire2012Status",         new Fire2012Effect(leds, N_PIXELS) },
};

constexpr uint8_t EFFECT_COUNT =
    sizeof(effectRegistry) / sizeof(effectRegistry[0]);

// ============================================================================
// PERSISTENCIA DE PARÁMETROS
// ============================================================================

void saveEffectParams() {
    DynamicJsonDocument doc(4096);
    for (uint8_t i = 0; i < EFFECT_COUNT; i++) {
        Effect* fx = effectRegistry[i].instance;
        if (!fx) continue;
        const char* name = effectRegistry[i].jsonName;
        JsonObject e = doc[name].to<JsonObject>();
        e["speed"]     = fx->getSpeed();
        e["intensity"] = fx->getIntensity();
        e["custom1"]   = fx->getCustom1();
        e["custom2"]   = fx->getCustom2();
        e["custom3"]   = fx->getCustom3();
        e["check1"]    = fx->getCheck1();
        e["check2"]    = fx->getCheck2();
        e["check3"]    = fx->getCheck3();
    }
    File f = LittleFS.open("/params.json", "w");
    if (f) {
        serializeJson(doc, f);
        f.close();
    }
}

void loadEffectParams() {
    if (!LittleFS.exists("/params.json")) return;
    File f = LittleFS.open("/params.json", "r");
    if (!f) return;
    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, f);
    if (err) {
        f.close();
        return;
    }
    for (uint8_t i = 0; i < EFFECT_COUNT; i++) {
        const char* name = effectRegistry[i].jsonName;
        if (!doc.containsKey(name)) continue;
        Effect* fx = effectRegistry[i].instance;
        if (!fx) continue;
        JsonObject e = doc[name];
        fx->setSpeed(e["speed"] | DEFAULT_SPEED);
        fx->setIntensity(e["intensity"] | DEFAULT_INTENSITY);
        fx->setCustom1(e["custom1"] | DEFAULT_C1);
        fx->setCustom2(e["custom2"] | DEFAULT_C2);
        fx->setCustom3(e["custom3"] | DEFAULT_C3);
        fx->setCheck1(e["check1"] | false);
        fx->setCheck2(e["check2"] | false);
        fx->setCheck3(e["check3"] | false);
    }
    f.close();
}
