#pragma once
#include "Effect.h"
#include "../state/AppState.h"
#include "PaletteManager.h"

// ──────────────────────────────────────────────────────────────────────────────
// Heartbeat — Latido cardíaco con decaimiento exponencial (WLED port)
// ──────────────────────────────────────────────────────────────────────────────
// Algoritmo WLED original (mode_heartbeat):
//   - BPM = 40 + (speed >> 3)         → 40-71 BPM
//   - msPerBeat = 60000 / bpm
//   - secondBeat (dub) a 1/3 del ciclo
//   - Brillo acumulado _bri se decae cada frame:
//       _bri = _bri * 2042 / (2048 + intensity)
//   - El latido resetea _bri a UINT16_MAX (brillo máximo)
//   - Color = blend(palette, secondary, 255 - (_bri >> 8))
//
// Params:
//   speed     → BPM base         (40 + speed/8)
//   intensity → Tasa de decaimiento (mayor = decae más rápido)
// ──────────────────────────────────────────────────────────────────────────────

class HeartbeatEffect : public Effect {
private:
    uint16_t _bri = 0;         // Brightness accumulator (0-65535, WLED aux1)
    uint32_t _lastBeat = 0;    // Timestamp of last beat
    bool     _secondBeat = false; // Whether dub already fired this cycle

public:
    static const char _meta[];

    HeartbeatEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
        _lastBeat = millis();
    }

    const char* getMeta() const override { return _meta; }

    void render() override {
        // ── WLED BPM formula: 40 + (speed >> 3) → 40-71 BPM ──────────────────
        unsigned bpm = 40 + (params.speed >> 3);
        uint32_t msPerBeat = 60000UL / bpm;
        uint32_t secondBeat = msPerBeat / 3;   // WLED: second beat at 33%
        if (secondBeat < 1) secondBeat = 1;

        uint32_t now = millis();
        uint32_t beatTimer = now - _lastBeat;

        // ── Beat timing — reset brightness BEFORE this frame's decay ──────────
        if (beatTimer > msPerBeat) {
            // Main beat (lub) — restore full brightness
            _bri = UINT16_MAX;
            _secondBeat = false;
            _lastBeat = now;
        } else if (beatTimer > secondBeat && !_secondBeat) {
            // Second beat (dub) — restore full brightness
            _bri = UINT16_MAX;
            _secondBeat = true;
        }

        // ── Exponential brightness decay (WLED formula) ──────────────────────
        //   bri_lower = bri_lower * 2042 / (2048 + intensity)
        //   Higher intensity → faster decay → shorter pulse
        uint32_t decayed = (uint32_t)_bri * 2042UL / (2048UL + params.intensity);
        _bri = (uint16_t)decayed;

        // ── Render: blend palette color toward secondary (dim/black) ──────────
        //   blendAmt = 255 - (_bri >> 8):
        //     _bri=65535 (just beat) → blendAmt=0     → 100% palette
        //     _bri=0     (decayed)   → blendAmt=255   → 100% secondary
        uint8_t blendAmt = 255 - (_bri >> 8);
        CRGBPalette16 pal = PaletteManager::getPalette(_paletteIndex);
        CRGB palColor = ColorFromPalette(pal, 0, stripLed.brightness, LINEARBLEND);
        CRGB secColor = CRGB::Black;  // same as WLED SEGCOLOR(1) = black default
        CRGB color = blend(palColor, secColor, blendAmt);
        fill_solid(leds, numLeds, color);

        FastLED.show();
    }
};

const char HeartbeatEffect::_meta[] =
    "Heartbeat@BPM,Decay;;;;sx=128,ix=128,pa=19";
