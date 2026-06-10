#pragma once
#include "Effect.h"
#include "../state/AppState.h"
#include "PaletteManager.h"

// ──────────────────────────────────────────────────────────────────────────────
// SinelonEffect — WLED Sinelon adapted for SML
// ──────────────────────────────────────────────────────────────────────────────
// Parameters (from metadata):
//   speed     → Beat frequency  (sx=23) — higher = faster movement
//   intensity → Trail fade rate (ix=128) — higher = shorter trail
// ──────────────────────────────────────────────────────────────────────────────

class SinelonEffect : public Effect {
private:
    static const char _meta[];
    uint8_t _hue = 0;

public:
    SinelonEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        // Speed controls beatsin16 frequency (movement speed)
        // Intensity controls trail fade rate
        uint8_t fade = map(params.intensity, 0, 255, 2, 200);

        fadeToBlackBy(leds, numLeds, fade);

        unsigned pos = beatsin16(params.speed, 0, numLeds - 1);
        leds[pos] = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), _hue, stripLed.brightness, LINEARBLEND);

        // Advance hue slowly for color cycling
        EVERY_N_MILLISECONDS(20) {
            _hue++;
        }

        FastLED.show();
    }
};

// Metadata: "Name@labels;defaults"
// Labels: speed, intensity, custom1, custom2, custom3, (reserved), check1, check2, check3
const char SinelonEffect::_meta[] =
    "Sinelon@Beat,Trail,,,,,,,,;;;;sx=23,ix=128";
