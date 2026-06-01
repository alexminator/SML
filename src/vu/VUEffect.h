// ──────────────────────────────────────────────────────────────────────────────
// VUEffect.h — Base class for VU meter effects
// ──────────────────────────────────────────────────────────────────────────────
// Encapsulates shared audio state (circular buffer, level detection, peak
// tracking).
// All VUEffect subclasses share no state between them; each has its own
// buffer since only one is active at a time in the EffectRegistry.
// ──────────────────────────────────────────────────────────────────────────────
#pragma once

#include "../effects/Effect.h"

class VUEffect : public Effect {
public:
    VUEffect(CRGB* l, uint16_t n)
        : Effect(l, n),
          _vol(),
          _volCount(0),
          _lvl(0),
          _minLvl(0),
          _maxLvl(512),
          _peak(0),
          _dotCount(0)
    {}

protected:
    // ── VU state ───────────────────────────────────────────────────────────
    uint16_t _vol[SAMPLES];   // circular sample buffer (replaces volLeft[])
    uint8_t  _volCount;       // current index into _vol (replaces volCountLeft)
    uint16_t _lvl;            // dampened reading (replaces lvlLeft)
    uint16_t _minLvl;         // dynamic minimum (replaces minLvlAvgLeft)
    uint16_t _maxLvl;         // dynamic maximum (replaces maxLvlAvgLeft)
    uint8_t  _peak;           // peak-dot position (replaces peakLeft)
    uint8_t  _dotCount;       // frames since last peak drop

    // ── Audio helpers ──────────────────────────────────────────────────────

    /// Read ADC, apply dampening, store in circular buffer, return bar height
    uint16_t auxReading() {
        int n = analogRead(AUDIO_IN_PIN);
        n = abs(n - BIAS - DC_OFFSET);
        n = (n <= NOISE) ? 0 : (n - NOISE);
        _lvl = ((_lvl * 7) + n) >> 3;
        _vol[_volCount] = n;
        _volCount = (_volCount + 1) % SAMPLES;
        uint16_t height = TOP * (_lvl - _minLvl) / (long)(_maxLvl - _minLvl);
        height = constrain(height, 0, TOP);
        return height;
    }

    /// Decay the peak marker each PEAK_FALL frames
    void dropPeak() {
        if (++_dotCount >= PEAK_FALL) {
            if (_peak > 0) _peak--;
            _dotCount = 0;
        }
    }

    /// Recalculate dynamic min/max levels from the circular buffer
    void averageReadings() {
        uint16_t minLvl, maxLvl;
        minLvl = maxLvl = _vol[0];
        for (int i = 1; i < SAMPLES; i++) {
            if (_vol[i] < minLvl) minLvl = _vol[i];
            else if (_vol[i] > maxLvl) maxLvl = _vol[i];
        }
        if ((maxLvl - minLvl) < TOP) maxLvl = minLvl + TOP;
        _minLvl = (_minLvl * 63 + minLvl) >> 6;
        _maxLvl = (_maxLvl * 63 + maxLvl) >> 6;
    }
};
