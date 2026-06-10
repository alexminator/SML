#pragma once
#include "Effect.h"
#include "../state/AppState.h"
#include "PaletteManager.h"

// ──────────────────────────────────────────────────────────────────────────────
// DualScanEffect — WLED mode_dual_scan (port completo)
// ──────────────────────────────────────────────────────────────────────────────
// Dos puntos que se mueven en direcciones opuestas (ping-pong).
// Algoritmo WLED scan(dual=true):
//   cycleTime = 750 + (255-speed)*150
//   perc = strip.now % cycleTime
//   prog = (perc * 65535) / cycleTime
//   size = 1 + ((intensity * SEGLEN) >> 9)
//   ledIndex = (prog * ((SEGLEN*2) - size*2)) >> 16
//   led_offset = abs(ledIndex - (SEGLEN - size))
//
// Parámetros:
//   speed     → Scan speed (sx=128) — higher = faster
//   intensity → Dot size (ix=64) — tamaño del punto
//   check2    → Overlay (m2=0) — no limpia el fondo
// ──────────────────────────────────────────────────────────────────────────────

class DualScanEffect : public Effect {
private:
    static const char _meta[];

public:
    DualScanEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        if (numLeds <= 1) return;

        // ── WLED scan(dual=true) ─────────────────────────────────────────────
        uint32_t cycleTime = 750 + (255 - params.speed) * 150;
        uint32_t perc = millis() % cycleTime;
        int prog = (perc * 65535) / cycleTime;
        int size = 1 + ((params.intensity * numLeds) >> 9);
        if (size < 1) size = 1;

        int ledIndex = (prog * ((int)(numLeds * 2) - size * 2)) >> 16;

        // ── Background ──────────────────────────────────────────────────────
        if (!params.check2) {
            fill_solid(leds, numLeds, CRGB::Black);
        }

        int led_offset = ledIndex - ((int)numLeds - size);
        led_offset = abs(led_offset);

        // ── Dual: segundo punto en dirección opuesta ─────────────────────────
        for (int j = led_offset; j < led_offset + size && j < (int)numLeds; j++) {
            unsigned i2 = numLeds - 1 - j;
            // Color secundario (complementario)
            uint8_t hue2 = uint8_t((millis() >> 9) + 128);
            leds[i2] = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), hue2, stripLed.brightness, LINEARBLEND);
        }

        // ── Primer punto ────────────────────────────────────────────────────
        for (int j = led_offset; j < led_offset + size && j < (int)numLeds; j++) {
            uint8_t hue = uint8_t(millis() >> 9);
            leds[j] = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), hue, stripLed.brightness, LINEARBLEND);
        }

        FastLED.show();
    }
};

const char DualScanEffect::_meta[] =
    "DualScan@Speed,Size:1:8,,,,,Overlay;;;;sx=128,ix=64,m2=0";
