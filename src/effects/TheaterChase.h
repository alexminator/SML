#pragma once
#include "Effect.h"
#include "../state/AppState.h"

// ──────────────────────────────────────────────────────────────────────────────
// TheaterChaseEffect — WLED mode_theater_chase (port completo)
// ──────────────────────────────────────────────────────────────────────────────
// Efecto teatro: luces que "corren" con espacios intermedios.
// Algoritmo WLED running():
//   width = 3 + (intensity >> 4)
//   cycleTime = 50 + (255 - speed)
//   it = strip.now / cycleTime
//   (i % width) == aux0 → color primario, sino color secundario
//
// Parámetros:
//   speed     → Chase speed (sx=128) — higher = faster
//   intensity → Gap size (ix=64) — espacio entre luces (3 + i/16)
// ──────────────────────────────────────────────────────────────────────────────

class TheaterChaseEffect : public Effect {
private:
    static const char _meta[];
    uint8_t _aux0 = 0;
    uint32_t _lastStep = 0;

public:
    TheaterChaseEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        if (numLeds <= 1) return;

        int width = 3 + (params.intensity >> 4);
        uint32_t cycleTime = 50 + (255 - params.speed);
        uint32_t it = millis() / cycleTime;

        // ── Render ──────────────────────────────────────────────────────────
        for (unsigned i = 0; i < numLeds; i++) {
            // Color primario (brillante) o secundario (oscuro/negro)
            if ((i % width) == _aux0) {
                // Luz encendida — color con hue progresivo
                leds[i] = CHSV((i * 16) + uint8_t(millis() >> 8), 255, stripLed.brightness);
            } else {
                // Apagado
                leds[i] = CRGB::Black;
            }
        }

        // ── Avanzar paso ───────────────────────────────────────────────────
        if (it != _lastStep) {
            _aux0 = (_aux0 + 1) % width;
            _lastStep = it;
        }

        FastLED.show();
    }
};

const char TheaterChaseEffect::_meta[] =
    "Theater@Speed,Gap:1:10;;;;;;;sx=128,ix=64";
