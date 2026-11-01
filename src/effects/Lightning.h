#pragma once
#include "Effect.h"
#include "../state/AppState.h"

// ──────────────────────────────────────────────────────────────────────────────
// LightningEffect — WLED mode_lightning (port)
// ──────────────────────────────────────────────────────────────────────────────
// Relámpagos aleatorios: un rayo líder seguido de flashes principales.
//
// Parámetros:
//   speed     → Intervalo entre tormentas (sx=128)
//   intensity → Número de flashes por tormenta (ix=128)
//   check1    → Overlay (m1=0) — true = no borra fondo
// ──────────────────────────────────────────────────────────────────────────────

class LightningEffect : public Effect {
private:
    static const char _meta[];
    uint8_t  _flashes;     // flashes remaining
    uint16_t _delay;       // delay between flashes (ms)
    uint32_t _lastFlash;   // timestamp of last flash
    uint16_t _ledStart;    // start position of current flash
    uint16_t _ledLen;      // length of current flash

public:
    LightningEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
        _flashes = 0;
        _delay = 0;
        _lastFlash = 0;
        _ledStart = 0;
        _ledLen = 0;
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        if (numLeds <= 1) return;

        // ── Color: el de la tira ─────────────────────────────────────────────
        // El rayo sólo muestreaba 1-2 puntos de la paleta (índice 32 fijo en el
        // líder y un tramo corto en el flash), así que elegir paleta no cambiaba
        // casi nada: usa el color global y la rueda lo recolorea en vivo.
        const CRGB bolt = CRGB(stripLed.R, stripLed.G, stripLed.B);

        // Background: if not overlay, fill with secondary color (black if no color)
        if (!params.check1) {
            fill_solid(leds, numLeds, CRGB::Black);
        }

        if (_flashes == 0) {
            // Init new lightning strike
            _ledStart = random16(numLeds);
            _ledLen = 1 + random16(numLeds - _ledStart);
            _flashes = 4 + random8(4 + params.intensity / 20);
            _flashes *= 2; // even count (leader + flashes)

            // Leader flash (dim)
            uint8_t bri = 52;
            CRGB leader = bolt;
            leader.nscale8(scale8(bri, stripLed.brightness));
            for (unsigned i = _ledStart; i < _ledStart + _ledLen && i < numLeds; i++) {
                leds[i] = leader;
            }
            _delay = 200;
            _lastFlash = millis();
            _flashes--;
        } else {
            if (millis() - _lastFlash >= _delay) {
                _flashes--;

                if (_flashes > 2 && (_flashes & 0x01)) {
                    // Main flash (bright)
                    uint8_t bri = 255 / random8(1, 3);
                    CRGB flash = bolt;
                    flash.nscale8(scale8(bri, stripLed.brightness));
                    for (unsigned i = _ledStart; i < _ledStart + _ledLen && i < numLeds; i++) {
                        leds[i] = flash;
                    }
                    _delay = 50 + random8(100);
                } else {
                    // Off period
                    for (unsigned i = _ledStart; i < _ledStart + _ledLen && i < numLeds; i++) {
                        if (!params.check1) leds[i] = CRGB::Black;
                    }
                    if (_flashes <= 2) {
                        // End of strike, long pause until next
                        _delay = random8(255 - params.speed) * 100;
                        if (_delay < 100) _delay = 100;
                    } else {
                        _delay = 50 + random8(100);
                    }
                }

                _lastFlash = millis();
            }
        }

        FastLED.show();
    }
};

// Sin pa=: el rayo es de un solo color (el de la tira), no de una paleta.
const char LightningEffect::_meta[] =
    "Lightning@Speed,Flashing,,,,,Overlay;;;;sx=128,ix=128";
