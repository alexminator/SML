#pragma once
#include "Effect.h"
#include "../state/AppState.h"

// ──────────────────────────────────────────────────────────────────────────────
// CometEffect — WLED-style bouncing comet for SML
// ──────────────────────────────────────────────────────────────────────────────
// Parameters (from metadata):
//   speed     → Movement speed (sx=64) — higher = faster comet
//   intensity → Trail glow (ix=128) — higher = más corto el rastro
//   custom1   → Comet size (c1=4) — number of LEDs in tail (1-8)
//   check1    → Blur (m1=0) — duplica la persistencia del trail
// ──────────────────────────────────────────────────────────────────────────────

class CometEffect : public Effect {
private:
    static const char _meta[];
    int16_t _pos = 0;
    int8_t  _dir = 1;
    uint8_t _hue = 0;

public:
    CometEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        // Speed → delay between steps (higher speed = shorter delay)
        uint16_t stepMs = map(params.speed, 0, 255, 150, 20);

        static unsigned long lastMove = 0;
        uint32_t now = millis();
        if (now - lastMove < stepMs) {
            FastLED.show();
            return;
        }
        lastMove = now;

        // Trail fade: intensity controls how much glow lingers
        // blur mode = softer trail (half the fade = double the persistence)
        uint8_t baseFade = map(params.intensity, 0, 255, 5, 120);
        uint8_t fade = params.check1 ? (baseFade / 2) : baseFade;
        fadeToBlackBy(leds, numLeds, fade);

        // Comet size (1-8 LEDs, slider range from metadata)
        uint8_t cometSize = constrain(params.custom1, 1, 8);

        // Advance hue for color cycling
        _hue += 4;

        // Bounce at strip edges
        _pos += _dir;
        int16_t maxPos = numLeds - cometSize;
        if (_pos > maxPos) { _pos = maxPos; _dir = -_dir; }
        if (_pos < 0) { _pos = 0; _dir = -_dir; }

        // Draw comet head-to-tail (head brightest, tail dimmest)
        for (int i = 0; i < cometSize; i++) {
            int p = _pos + i;
            if (p >= 0 && p < numLeds) {
                uint8_t bright = map(i, 0, cometSize - 1,
                                     stripLed.brightness,
                                     (uint8_t)(stripLed.brightness / 4));
                leds[p] = CHSV(_hue, 255, bright);
            }
        }

        FastLED.show();
    }
};

// Metadata: "Name@labels;defaults"
// Labels: speed, intensity, custom1, custom2, custom3, (reserved), check1, check2, check3
const char CometEffect::_meta[] =
    "Comet@Speed,Glow,Size:1:8,,,,,Blur,,,,;;;sx=64,ix=128,c1=4,m1=0";
