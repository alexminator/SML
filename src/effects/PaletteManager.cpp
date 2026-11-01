// ──────────────────────────────────────────────────────────────────────────────
// PaletteManager.cpp — Definición de las 18 paletas de colores
// ──────────────────────────────────────────────────────────────────────────────
#include "PaletteManager.h"

// ── Storage ──────────────────────────────────────────────────────────────────
CRGBPalette16 PaletteManager::_palettes[PALETTE_COUNT];
const char*   PaletteManager::_names[PALETTE_COUNT];
bool          PaletteManager::_initialized = false;

// ──────────────────────────────────────────────────────────────────────────────
void PaletteManager::ensureInit() {
    if (_initialized) return;

    // ── Built-in FastLED ───────────────────────────────────────────────────
    _palettes[0]  = RainbowColors_p;
    _palettes[1]  = PartyColors_p;
    _palettes[2]  = HeatColors_p;
    _palettes[3]  = LavaColors_p;
    _palettes[4]  = ForestColors_p;
    _palettes[5]  = OceanColors_p;
    _palettes[6]  = CloudColors_p;
    _palettes[7]  = RainbowStripeColors_p;

    // ── 8. Fire — negro → rojo puro → naranja → amarillo (sin blanco: el blanco apaga el resto)
    _palettes[8] = CRGBPalette16(
        CRGB(12,0,0), CRGB(28,2,0), CRGB(44,6,0), CRGB(61,12,0),
        CRGB(77,20,0), CRGB(93,31,0), CRGB(109,43,0), CRGB(125,58,0),
        CRGB(142,74,0), CRGB(158,93,0), CRGB(174,114,0), CRGB(190,137,0),
        CRGB(206,163,0), CRGB(223,190,0), CRGB(239,219,0), CRGB(255,251,0)
    );

    // ── 9. Sunset — magenta profundo → rojo → naranja (atardecer, sin llegar al blanco)
    _palettes[9] = CRGBPalette16(
        CRGB(74,0,80), CRGB(92,0,88), CRGB(103,0,87), CRGB(115,0,83),
        CRGB(127,0,76), CRGB(138,0,67), CRGB(150,0,55), CRGB(162,0,40),
        CRGB(173,0,22), CRGB(185,0,2), CRGB(197,22,0), CRGB(208,48,0),
        CRGB(220,76,0), CRGB(232,108,0), CRGB(243,142,0), CRGB(255,179,0)
    );

    // ── 10. Aurora Cyan — turquesa → azul (el remate frío de una aurora, sin blanco)
    _palettes[10] = CRGBPalette16(
        CRGB(0,95,100), CRGB(0,97,110), CRGB(0,98,121), CRGB(0,97,131),
        CRGB(0,95,141), CRGB(0,91,152), CRGB(0,86,162), CRGB(0,79,172),
        CRGB(0,71,183), CRGB(0,62,193), CRGB(0,51,203), CRGB(0,38,214),
        CRGB(0,24,224), CRGB(0,9,234), CRGB(8,0,245), CRGB(26,0,255)
    );

    // ── 11. Candy — rosa → magenta → violeta → turquesa → amarillo (todo saturado)
    _palettes[11] = CRGBPalette16(
        CRGB(255,0,191), CRGB(255,0,143), CRGB(255,0,96), CRGB(255,0,48),
        CRGB(255,0,0), CRGB(255,48,0), CRGB(255,96,0), CRGB(255,120,0),
        CRGB(0,255,255), CRGB(0,255,207), CRGB(0,255,159), CRGB(0,255,112),
        CRGB(128,255,0), CRGB(175,255,0), CRGB(223,255,0), CRGB(255,239,0)
    );

    // ── 12. C9 — rojo, verde, ámbar y azul de bombilla navideña (cuatro puros)
    _palettes[12] = CRGBPalette16(
        CRGB(255,0,0), CRGB(255,0,0), CRGB(255,0,0), CRGB(255,0,0),
        CRGB(0,255,0), CRGB(0,255,0), CRGB(0,255,0), CRGB(0,255,0),
        CRGB(255,160,0), CRGB(255,160,0), CRGB(255,160,0), CRGB(255,160,0),
        CRGB(0,60,255), CRGB(0,60,255), CRGB(0,60,255), CRGB(0,60,255)
    );

    // ── 13. Amber/Blue — pareja complementaria en bloques: ámbar y azul puros
    _palettes[13] = CRGBPalette16(
        CRGB(255,140,0), CRGB(255,140,0), CRGB(255,140,0), CRGB(255,140,0),
        CRGB(0,90,255), CRGB(0,90,255), CRGB(0,90,255), CRGB(0,90,255),
        CRGB(255,140,0), CRGB(255,140,0), CRGB(255,140,0), CRGB(255,140,0),
        CRGB(0,90,255), CRGB(0,90,255), CRGB(0,90,255), CRGB(0,90,255)
    );

    // ── 14. Pink/Lime — pareja complementaria en bloques: rosa y lima puros
    _palettes[14] = CRGBPalette16(
        CRGB(255,0,130), CRGB(255,0,130), CRGB(255,0,130), CRGB(255,0,130),
        CRGB(140,255,0), CRGB(140,255,0), CRGB(140,255,0), CRGB(140,255,0),
        CRGB(255,0,130), CRGB(255,0,130), CRGB(255,0,130), CRGB(255,0,130),
        CRGB(140,255,0), CRGB(140,255,0), CRGB(140,255,0), CRGB(140,255,0)
    );

    // ── 15. Violet/Yellow — pareja complementaria en bloques: violeta y amarillo puros
    _palettes[15] = CRGBPalette16(
        CRGB(140,0,255), CRGB(140,0,255), CRGB(140,0,255), CRGB(140,0,255),
        CRGB(255,230,0), CRGB(255,230,0), CRGB(255,230,0), CRGB(255,230,0),
        CRGB(140,0,255), CRGB(140,0,255), CRGB(140,0,255), CRGB(140,0,255),
        CRGB(255,230,0), CRGB(255,230,0), CRGB(255,230,0), CRGB(255,230,0)
    );

    // ── 16. Blue-Cyan — azul → turquesa (sin blanco)
    _palettes[16] = CRGBPalette16(
        CRGB(0,12,90), CRGB(0,17,101), CRGB(0,24,112), CRGB(0,31,123),
        CRGB(0,39,134), CRGB(0,48,145), CRGB(0,57,156), CRGB(0,68,167),
        CRGB(0,79,178), CRGB(0,92,189), CRGB(0,105,200), CRGB(0,119,211),
        CRGB(0,134,222), CRGB(0,149,233), CRGB(0,166,244), CRGB(0,183,255)
    );

    // ── 17. Red-Purple — rojo puro → magenta → violeta (sin rosados pálidos)
    _palettes[17] = CRGBPalette16(
        CRGB(70,0,0), CRGB(82,0,7), CRGB(95,0,15), CRGB(107,0,26),
        CRGB(119,0,38), CRGB(132,0,52), CRGB(144,0,69), CRGB(156,0,87),
        CRGB(169,0,108), CRGB(181,0,130), CRGB(193,0,154), CRGB(206,0,180),
        CRGB(218,0,208), CRGB(222,0,230), CRGB(215,0,243), CRGB(205,0,255)
    );

    // ── 18. Fire Blue — fuego azul: negro → azul → turquesa
    _palettes[18] = CRGBPalette16(
        CRGB(0,0,12), CRGB(0,2,28), CRGB(0,5,44), CRGB(0,9,61),
        CRGB(0,16,77), CRGB(0,23,93), CRGB(0,32,109), CRGB(0,43,125),
        CRGB(0,55,142), CRGB(0,69,158), CRGB(0,84,174), CRGB(0,101,190),
        CRGB(0,119,206), CRGB(0,139,223), CRGB(0,160,239), CRGB(0,183,255)
    );

    // ── 19. Fire Green — fuego verde: negro → verde → lima
    _palettes[19] = CRGBPalette16(
        CRGB(0,12,4), CRGB(0,28,8), CRGB(0,44,10), CRGB(0,61,11),
        CRGB(0,77,10), CRGB(0,93,7), CRGB(0,109,2), CRGB(5,125,0),
        CRGB(13,142,0), CRGB(23,158,0), CRGB(35,174,0), CRGB(49,190,0),
        CRGB(64,206,0), CRGB(82,223,0), CRGB(101,239,0), CRGB(122,255,0)
    );

    // ── 20. Fire Violet — fuego violeta: negro → violeta → magenta
    _palettes[20] = CRGBPalette16(
        CRGB(8,0,12), CRGB(21,0,28), CRGB(35,0,44), CRGB(50,0,61),
        CRGB(67,0,77), CRGB(86,0,93), CRGB(106,0,109), CRGB(125,0,123),
        CRGB(142,0,133), CRGB(158,0,141), CRGB(174,0,147), CRGB(190,0,152),
        CRGB(206,0,155), CRGB(223,0,157), CRGB(239,0,157), CRGB(255,0,155)
    );

    // ── 21. Aurora Green — verde → turquesa: la aurora boreal de verdad, sin blancos
    _palettes[21] = CRGBPalette16(
        CRGB(0,90,22), CRGB(0,101,31), CRGB(0,112,42), CRGB(0,123,53),
        CRGB(0,134,66), CRGB(0,145,80), CRGB(0,156,96), CRGB(0,167,113),
        CRGB(0,178,131), CRGB(0,189,151), CRGB(0,200,172), CRGB(0,211,194),
        CRGB(0,222,218), CRGB(0,223,233), CRGB(0,219,244), CRGB(0,213,255)
    );

    // ── 22. Aurora Violet — violeta → rosa: el remate morado de la aurora
    _palettes[22] = CRGBPalette16(
        CRGB(30,0,90), CRGB(40,0,101), CRGB(52,0,112), CRGB(64,0,123),
        CRGB(79,0,134), CRGB(94,0,145), CRGB(111,0,156), CRGB(129,0,167),
        CRGB(149,0,178), CRGB(170,0,189), CRGB(192,0,200), CRGB(211,0,206),
        CRGB(222,0,203), CRGB(233,0,198), CRGB(244,0,193), CRGB(255,0,185)
    );

    // ── 23. Candle — llama de vela: negro → ámbar → amarillo cálido
    _palettes[23] = CRGBPalette16(
        CRGB(12,7,0), CRGB(28,17,0), CRGB(44,29,0), CRGB(61,41,0),
        CRGB(77,55,0), CRGB(93,69,0), CRGB(109,84,0), CRGB(125,101,0),
        CRGB(142,118,0), CRGB(158,137,0), CRGB(174,156,0), CRGB(190,177,0),
        CRGB(206,198,0), CRGB(223,221,0), CRGB(233,239,0), CRGB(241,255,0)
    );

    // ── 24. Sunrise — naranja → dorado → amarillo (amanecer, sin blanco)
    _palettes[24] = CRGBPalette16(
        CRGB(60,28,0), CRGB(73,37,0), CRGB(86,47,0), CRGB(99,58,0),
        CRGB(112,70,0), CRGB(125,83,0), CRGB(138,97,0), CRGB(151,112,0),
        CRGB(164,128,0), CRGB(177,145,0), CRGB(190,163,0), CRGB(203,182,0),
        CRGB(216,202,0), CRGB(229,224,0), CRGB(238,242,0), CRGB(241,255,0)
    );

    // ── 25. Ocean Emerald — verde mar → turquesa (sin llegar al blanco)
    _palettes[25] = CRGBPalette16(
        CRGB(0,80,28), CRGB(0,92,37), CRGB(0,103,47), CRGB(0,115,58),
        CRGB(0,127,71), CRGB(0,138,85), CRGB(0,150,101), CRGB(0,162,117),
        CRGB(0,173,135), CRGB(0,185,155), CRGB(0,197,175), CRGB(0,208,197),
        CRGB(0,220,220), CRGB(0,219,232), CRGB(0,217,243), CRGB(0,213,255)
    );

    // ── 26. Ocean Violet — azul marino → violeta (mar de noche)
    _palettes[26] = CRGBPalette16(
        CRGB(0,29,80), CRGB(0,26,92), CRGB(0,22,103), CRGB(0,15,115),
        CRGB(0,7,127), CRGB(3,0,138), CRGB(15,0,150), CRGB(29,0,162),
        CRGB(45,0,173), CRGB(62,0,185), CRGB(81,0,197), CRGB(103,0,208),
        CRGB(125,0,220), CRGB(150,0,232), CRGB(177,0,243), CRGB(205,0,255)
    );

    // ── Nombres ──    // ── Nombres ────────────────────────────────────────────────────────────
    _names[0]  = "Rainbow";
    _names[1]  = "Party";
    _names[2]  = "Heat";
    _names[3]  = "Lava";
    _names[4]  = "Forest";
    _names[5]  = "Ocean";
    _names[6]  = "Cloud";
    _names[7]  = "Rainbow Stripe";
    _names[8]  = "Fire";
    _names[9]  = "Sunset";
    _names[10] = "Aurora Cyan";
    _names[11] = "Candy";
    _names[12] = "C9";
    _names[13] = "Amber/Blue";
    _names[14] = "Pink/Lime";
    _names[15] = "Violet/Yellow";
    _names[16] = "Blue-Cyan";
    _names[17] = "Red-Purple";
    _names[18] = "Fire Blue";
    _names[19] = "Fire Green";
    _names[20] = "Fire Violet";
    _names[21] = "Aurora Green";
    _names[22] = "Aurora Violet";
    _names[23] = "Candle";
    _names[24] = "Sunrise";
    _names[25] = "Ocean Emerald";
    _names[26] = "Ocean Violet";

    _initialized = true;
}

// ──────────────────────────────────────────────────────────────────────────────
const CRGBPalette16& PaletteManager::getPalette(uint8_t index) {
    ensureInit();
    if (index >= PALETTE_COUNT) index = 0;
    return _palettes[index];
}

// ──────────────────────────────────────────────────────────────────────────────
const char* PaletteManager::getName(uint8_t index) {
    ensureInit();
    if (index >= PALETTE_COUNT) index = 0;
    return _names[index];
}

// ──────────────────────────────────────────────────────────────────────────────
// ──────────────────────────────────────────────────────────────────────────────
void PaletteManager::getSwatch(uint8_t paletteIndex, CRGB* swatch, uint8_t len) {
    ensureInit();
    if (len == 0) return;
    if (paletteIndex >= PALETTE_COUNT) paletteIndex = 0;

    const CRGBPalette16& pal = _palettes[paletteIndex];
    for (uint8_t i = 0; i < len; i++) {
        uint8_t index8 = (i * 255) / (len - 1);
        swatch[i] = ColorFromPalette(pal, index8, 255, LINEARBLEND);
    }
}
