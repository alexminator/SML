// ──────────────────────────────────────────────────────────────────────────────
// PaletteManager.h — Sistema de paletas de colores configurables
// ──────────────────────────────────────────────────────────────────────────────
// Inspirado en WLED: cada efecto puede usar cualquier paleta definida aquí.
// Uso:
//   #include "PaletteManager.h"
//   const CRGBPalette16& pal = PaletteManager::getPalette(paletteIndex);
//   leds[i] = ColorFromPalette(pal, hue, bright, LINEARBLEND);
// ──────────────────────────────────────────────────────────────────────────────
#pragma once

#include <FastLED.h>

// 27 paletas fijas + 1 (índice 27) que se construye con el color de la lámpara:
// la usan los efectos VU, que sólo cambian de color (no de paleta).
#define PALETTE_COUNT 28
#define PALETTE_LAMP_COLOR 27

class PaletteManager {
public:
    /// Retorna la paleta CRGBPalette16 para el índice dado (0 = Rainbow default)
    static const CRGBPalette16& getPalette(uint8_t index);

    /// Reconstruye la paleta 27 ("Lamp color") a partir de un color: rampa del
    /// mismo tono, sin negros, para que los VU mantengan su degradado.
    static void setBaseColor(const CRGB& color);

    /// Retorna el nombre legible de la paleta
    static const char* getName(uint8_t index);

    /// Retorna cuántas paletas hay definidas
    static uint8_t count() { return PALETTE_COUNT; }

    /// Llena un array de CRGB con los colores representativos de la paleta
    /// (para mostrar el swatch en el frontend)
    static void getSwatch(uint8_t paletteIndex, CRGB* swatch, uint8_t len);

private:
    static CRGBPalette16 _palettes[];
    static const char* _names[];
    static bool _initialized;

    static void ensureInit();
};
