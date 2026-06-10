#pragma once
#include "Effect.h"
#include "../state/AppState.h"
#include "PaletteManager.h"

// ──────────────────────────────────────────────────────────────────────────────
// AuroraEffect — WLED mode_aurora (port)
// ──────────────────────────────────────────────────────────────────────────────
// Aurora boreal simulada con múltiples "olas" de luz que se desplazan,
// mezclándose suavemente.
//
// Parámetros:
//   speed     → Velocidad de olas (sx=24)
//   intensity → Cantidad de olas (ix=128) — higher = más olas simultáneas
// ──────────────────────────────────────────────────────────────────────────────

// ── Constantes Aurora ─────────────────────────────────────────────────────────
#define AW_SHIFT 16
#define AW_SCALE (1 << AW_SHIFT)
#define W_MAX_COUNT 12
#define W_MAX_SPEED 6
#define W_WIDTH_FACTOR 6

class AuroraWave {
private:
    int32_t  center;
    uint32_t ageFactor_cached;
    uint16_t ttl;
    uint16_t age;
    uint16_t width;
    uint16_t basealpha;       // scaled by AW_SCALE
    uint16_t speed_factor;     // scaled by AW_SCALE
    int16_t  wave_start;
    int16_t  wave_end;
    bool     goingleft;
    bool     _alive;
    CRGB     basecolor;

public:
    AuroraWave() : _alive(false) {}

    void init(uint32_t segment_length, CRGB color) {
        ttl = random16(500, 1501);
        basecolor = color;
        basealpha = random8(60, 100) * AW_SCALE / 100;
        age = 0;
        width = random16(segment_length / 20, segment_length / W_WIDTH_FACTOR) + 1;
        center = (((uint32_t)random8(101) << AW_SHIFT) / 100) * segment_length;
        goingleft = random8() & 0x01;
        speed_factor = (((uint32_t)random8(10, 31) * W_MAX_SPEED) << AW_SHIFT) / (100 * 255);
        _alive = true;
    }

    void updateCachedValues() {
        uint32_t half_ttl = ttl >> 1;
        if (age < half_ttl) {
            ageFactor_cached = ((uint32_t)age << AW_SHIFT) / half_ttl;
        } else {
            ageFactor_cached = ((uint32_t)(ttl - age) << AW_SHIFT) / half_ttl;
        }
        if (ageFactor_cached >= AW_SCALE) ageFactor_cached = AW_SCALE - 1;

        uint32_t center_led = center >> AW_SHIFT;
        wave_start = (int16_t)center_led - (int16_t)width;
        wave_end   = (int16_t)center_led + (int16_t)width;
    }

    CRGB getColorForLED(int ledIndex) {
        if (ledIndex < wave_start || ledIndex > wave_end) return CRGB(0, 0, 0);

        int32_t ledIndex_scaled = (int32_t)ledIndex << AW_SHIFT;
        int32_t offset = ledIndex_scaled - center;
        if (offset < 0) offset = -offset;
        uint32_t offsetFactor = offset / width;
        if (offsetFactor > AW_SCALE) return CRGB(0, 0, 0);

        uint32_t brightness_factor = (AW_SCALE - offsetFactor);
        brightness_factor = (brightness_factor * ageFactor_cached) >> AW_SHIFT;
        brightness_factor = (brightness_factor * basealpha) >> AW_SHIFT;

        CRGB rgb;
        rgb.r = (basecolor.r * brightness_factor) >> AW_SHIFT;
        rgb.g = (basecolor.g * brightness_factor) >> AW_SHIFT;
        rgb.b = (basecolor.b * brightness_factor) >> AW_SHIFT;

        return rgb;
    }

    void update(uint32_t segment_length, uint32_t speed) {
        int32_t step = speed_factor * speed;
        center += goingleft ? -step : step;
        age++;

        if (age > ttl) {
            _alive = false;
        } else {
            uint32_t width_scaled = (uint32_t)width << AW_SHIFT;
            uint32_t segment_length_scaled = segment_length << AW_SHIFT;

            if (goingleft) {
                if (center < -(int32_t)width_scaled) _alive = false;
            } else {
                if (center > (int32_t)segment_length_scaled + (int32_t)width_scaled) _alive = false;
            }
        }
    }

    bool stillAlive() { return _alive; }
};

class AuroraEffect : public Effect {
private:
    static const char _meta[];
    AuroraWave _waves[W_MAX_COUNT];
    uint8_t    _waveCount;

public:
    AuroraEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
        _waveCount = 2;
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        if (numLeds <= 1) return;

        _waveCount = map(params.intensity, 0, 255, 2, W_MAX_COUNT);

        for (int i = 0; i < _waveCount; i++) {
            _waves[i].update(numLeds, params.speed);
            if (!_waves[i].stillAlive()) {
                uint8_t hue = random8();
                _waves[i].init(numLeds, ColorFromPalette(PaletteManager::getPalette(_paletteIndex), hue, 255, LINEARBLEND));
            }
            _waves[i].updateCachedValues();
        }

        for (unsigned i = 0; i < numLeds; i++) {
            CRGB mixedRgb = CRGB(0, 0, 0);

            for (int j = 0; j < _waveCount; j++) {
                CRGB c = _waves[j].getColorForLED(i);
                mixedRgb.r = qadd8(mixedRgb.r, c.r);
                mixedRgb.g = qadd8(mixedRgb.g, c.g);
                mixedRgb.b = qadd8(mixedRgb.b, c.b);
            }

            leds[i] = mixedRgb;
            leds[i].nscale8(stripLed.brightness);
        }

        FastLED.show();
    }
};

const char AuroraEffect::_meta[] =
    "Aurora@Speed,Waves;;;;sx=24";
