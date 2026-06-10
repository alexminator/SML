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

    // ── 8. Fire (negro → rojo → naranja → amarillo → blanco) ──────────────
    _palettes[8]  = CRGBPalette16(
        CRGB::Black,       CRGB::Maroon,    CRGB::Red,       CRGB::Red,
        CRGB::OrangeRed,   CRGB::Orange,    CRGB::Gold,      CRGB::Gold,
        CRGB::Yellow,      CRGB::LightYellow, CRGB::White,   CRGB::White,
        CRGB::White,       CRGB::White,     CRGB::White,     CRGB::White
    );

    // ── 9. Sunset (negro → rojo oscuro → naranja → dorado) ─────────────────
    _palettes[9]  = CRGBPalette16(
        CRGB::Black,       CRGB(60,0,0),    CRGB(120,0,0),  CRGB::DarkRed,
        CRGB::OrangeRed,   CRGB::Orange,    CRGB::DarkOrange, CRGB::Orange,
        CRGB::Gold,        CRGB::Gold,      CRGB::Yellow,   CRGB::LightYellow,
        CRGB(255,240,200), CRGB(255,245,220), CRGB(255,250,240), CRGB::White
    );

    // ── 10. Aurora (oscuro → cian → verde → blanco verdoso) ───────────────
    _palettes[10] = CRGBPalette16(
        CRGB(0,5,15),      CRGB(0,20,40),   CRGB(0,50,90),   CRGB(0,80,120),
        CRGB(0,120,140),   CRGB(20,160,150), CRGB(60,200,160), CRGB(100,220,170),
        CRGB(140,240,180), CRGB(180,250,200), CRGB(200,255,210), CRGB(220,255,220),
        CRGB(240,255,240), CRGB(250,255,250), CRGB::White,    CRGB::White
    );

    // ── 11. Candy (rosa → magenta → cian → amarillo) ──────────────────────
    _palettes[11] = CRGBPalette16(
        CRGB::DeepPink,    CRGB::HotPink,   CRGB::Magenta,   CRGB::Magenta,
        CRGB::Purple,      CRGB::MediumPurple, CRGB::Violet, CRGB::Cyan,
        CRGB::Aqua,        CRGB::Aquamarine, CRGB::YellowGreen, CRGB::Yellow,
        CRGB::Gold,        CRGB::Orange,    CRGB::HotPink,   CRGB::DeepPink
    );

    // ── 12. C9 (navideño clásico: rojo, verde, dorado, azul) ──────────────
    _palettes[12] = CRGBPalette16(
        CRGB::Red,         CRGB::Red,       CRGB::Green,     CRGB::Green,
        CRGB::Gold,        CRGB::Gold,      CRGB::Blue,      CRGB::Blue,
        CRGB::Red,         CRGB::Red,       CRGB::Green,     CRGB::Green,
        CRGB::Gold,        CRGB::Gold,      CRGB::Blue,      CRGB::Blue
    );

    // ── 13. Ice (azul profundo → azul → blanco) ───────────────────────────
    _palettes[13] = CRGBPalette16(
        CRGB(0,10,30),     CRGB(0,20,60),   CRGB(0,40,100),  CRGB(0,70,140),
        CRGB(0,100,180),   CRGB(50,140,210), CRGB(100,180,230), CRGB(150,210,240),
        CRGB(180,230,250), CRGB(210,240,255), CRGB(230,248,255), CRGB::White,
        CRGB::White,       CRGB::White,     CRGB::White,     CRGB::White
    );

    // ── 14. Fairy (pasteles: rosa, lavanda, celeste, menta) ───────────────
    _palettes[14] = CRGBPalette16(
        CRGB(255,200,220), CRGB(240,180,230), CRGB(220,160,240), CRGB(200,180,255),
        CRGB(180,200,255), CRGB(160,220,255), CRGB(180,240,240), CRGB(200,255,220),
        CRGB(220,255,200), CRGB(255,250,200), CRGB(255,240,210), CRGB(255,220,220),
        CRGB(255,200,220), CRGB(240,180,230), CRGB(220,160,240), CRGB(200,180,255)
    );

    // ── 15. Beach (arena → turquesa → azul cielo) ─────────────────────────
    _palettes[15] = CRGBPalette16(
        CRGB(240,220,170), CRGB(230,210,160), CRGB(210,190,140), CRGB(180,200,180),
        CRGB(120,200,200), CRGB(80,190,210),  CRGB(60,180,220),  CRGB(50,170,220),
        CRGB(60,180,230),  CRGB(80,200,240),  CRGB(120,210,245), CRGB(160,220,250),
        CRGB(180,230,250), CRGB(200,240,255), CRGB(220,245,255), CRGB(240,250,255)
    );

    // ── 16. Blue-Cyan (azul → cian → blanco) ──────────────────────────────
    _palettes[16] = CRGBPalette16(
        CRGB(0,0,40),      CRGB(0,0,80),     CRGB(0,20,120),  CRGB(0,50,160),
        CRGB(0,80,200),    CRGB(0,120,220),  CRGB(0,160,240), CRGB(0,200,250),
        CRGB(40,220,255),  CRGB(80,235,255), CRGB(130,245,255), CRGB(180,250,255),
        CRGB(210,252,255), CRGB(235,254,255), CRGB(250,255,255), CRGB::White
    );

    // ── 17. Red-Purple (rojo → púrpura → rosa) ────────────────────────────
    _palettes[17] = CRGBPalette16(
        CRGB(40,0,0),      CRGB(80,0,20),    CRGB(120,0,40),  CRGB(160,0,60),
        CRGB(200,0,80),    CRGB(220,20,100), CRGB(240,40,120), CRGB(250,60,140),
        CRGB(255,80,160),  CRGB(255,100,180), CRGB(255,130,200), CRGB(255,160,210),
        CRGB(255,190,220), CRGB(255,210,235), CRGB(255,230,245), CRGB(255,245,250)
    );

    // ════════════════════════════════════════════════════════════════════════════
    // EFFECT DEFAULT PALETTES — coinciden con los colores originales de cada
    // efecto. Se usan como paleta por defecto del efecto correspondiente.
    // ════════════════════════════════════════════════════════════════════════════

    // ── 18. Fire Default (negro → rojo → amarillo → blanco) ──────────────────
    //   Original Fire.h: CRGBPalette16(Black, Red, Yellow, White)
    _palettes[18] = CRGBPalette16(
        CRGB::Black,       CRGB(40,0,0),     CRGB(80,0,0),     CRGB(130,0,0),
        CRGB::Red,         CRGB::Red,        CRGB(210,130,0),  CRGB::Gold,
        CRGB::Yellow,      CRGB::Yellow,     CRGB(255,255,200), CRGB::LightYellow,
        CRGB::White,       CRGB::White,      CRGB::White,      CRGB::White
    );

    // ── 19. Heartbeat Default (rojo pulsante) ─────────────────────────────────
    //   Original: CHSV(0,255,brightness) → rojo
    _palettes[19] = CRGBPalette16(
        CRGB::Red,         CRGB::Red,        CRGB::Red,        CRGB::Red,
        CRGB::Red,         CRGB::DarkRed,    CRGB(80,0,0),     CRGB(30,0,0),
        CRGB::Black,       CRGB::Black,      CRGB::Black,      CRGB::Black,
        CRGB::Black,       CRGB::Black,      CRGB::Black,      CRGB::Black
    );

    // ── 20. ICU Default (verde monitor) ──────────────────────────────────────
    //   Original: CHSV(120,255,brightness) → verde
    _palettes[20] = CRGBPalette16(
        CRGB(0,10,0),      CRGB(0,30,0),     CRGB(0,60,0),     CRGB(0,120,0),
        CRGB(0,200,0),     CRGB(0,255,0),    CRGB(0,255,0),    CRGB(0,200,0),
        CRGB(0,150,0),     CRGB(0,100,0),    CRGB(0,60,0),     CRGB(0,30,0),
        CRGB(0,10,0),      CRGB::Black,      CRGB::Black,      CRGB::Black
    );

    // ── 21. RWB Default (rojo → blanco → azul) ───────────────────────────────
    //   Original: Red (hue 0), White (hue 85), Blue (hue 170)
    _palettes[21] = CRGBPalette16(
        CRGB::Red,         CRGB::Red,        CRGB::Red,        CRGB::Red,
        CRGB::Red,         CRGB::White,      CRGB::White,      CRGB::White,
        CRGB::White,       CRGB::White,      CRGB::Blue,       CRGB::Blue,
        CRGB::Blue,        CRGB::Blue,       CRGB::Blue,       CRGB::Blue
    );

    // ── 22. Drip Default (azul agua) ─────────────────────────────────────────
    //   Original: CHSV(160,200,255) → azul/cian
    _palettes[22] = CRGBPalette16(
        CRGB(0,0,30),      CRGB(0,10,60),    CRGB(0,30,100),   CRGB(0,60,150),
        CRGB(0,100,200),   CRGB(0,150,230),  CRGB(20,180,255), CRGB(60,200,255),
        CRGB(100,215,255), CRGB(150,230,255),CRGB(200,245,255),CRGB(230,250,255),
        CRGB(245,255,255), CRGB::White,      CRGB::White,      CRGB::White
    );

    // ── 23. Candle Default (llama cálida ámbar) ──────────────────────────────
    //   Original: HeatColors_p style (negro → rojo → naranja → amarillo → blanco)
    _palettes[23] = CRGBPalette16(
        CRGB::Black,       CRGB(40,10,0),    CRGB(80,20,0),    CRGB(130,40,0),
        CRGB(180,70,0),    CRGB(220,110,0),  CRGB(240,150,0),  CRGB(255,190,30),
        CRGB(255,220,60),  CRGB(255,240,100),CRGB(255,250,150),CRGB(255,255,200),
        CRGB::White,       CRGB::White,      CRGB::White,      CRGB::White
    );

    // ── 24. Sunrise Default (amanecer rojo → naranja → amarillo → blanco) ───
    //   Original: Staged CHSV: hue 0→10→30→50, sat 255→0
    _palettes[24] = CRGBPalette16(
        CRGB(40,0,0),      CRGB(80,0,0),     CRGB(130,20,0),   CRGB(180,50,0),
        CRGB(220,80,0),    CRGB(255,120,0),  CRGB(255,160,20), CRGB(255,195,50),
        CRGB(255,215,80),  CRGB(255,230,120),CRGB(255,240,160),CRGB(255,245,200),
        CRGB(255,250,220), CRGB(255,252,240),CRGB::White,      CRGB::White
    );

    // ── 25. Halloween Default (blanco → gris → negro — ojos espeluznantes) ───
    //   Original: CRGB::White eye color
    _palettes[25] = CRGBPalette16(
        CRGB::White,       CRGB::White,      CRGB::White,      CRGB::White,
        CRGB::White,       CRGB::White,      CRGB(200,200,200),CRGB(150,150,150),
        CRGB(100,100,100), CRGB(60,60,60),   CRGB(30,30,30),   CRGB::Black,
        CRGB::Black,       CRGB::Black,      CRGB::Black,      CRGB::Black
    );

    // ── 26. Scanner Default (blanco → gris → negro — barrido KITT) ──────────
    //   Original: CRGB::White dot → CRGB(128,128,128) trail → black
    _palettes[26] = CRGBPalette16(
        CRGB::White,       CRGB::White,      CRGB(200,200,200),CRGB(140,140,140),
        CRGB(90,90,90),    CRGB(50,50,50),   CRGB(20,20,20),   CRGB::Black,
        CRGB::Black,       CRGB::Black,      CRGB::Black,      CRGB::Black,
        CRGB::Black,       CRGB::Black,      CRGB::Black,      CRGB::Black
    );

    // ── Nombres ────────────────────────────────────────────────────────────
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
    _names[10] = "Aurora";
    _names[11] = "Candy";
    _names[12] = "C9";
    _names[13] = "Ice";
    _names[14] = "Fairy";
    _names[15] = "Beach";
    _names[16] = "Blue-Cyan";
    _names[17] = "Red-Purple";
    _names[18] = "Fire Default";
    _names[19] = "Heartbeat Def.";
    _names[20] = "ICU Default";
    _names[21] = "RWB Default";
    _names[22] = "Drip Default";
    _names[23] = "Candle Default";
    _names[24] = "Sunrise Default";
    _names[25] = "Halloween Def.";
    _names[26] = "Scanner Default";

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
