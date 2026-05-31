// ──────────────────────────────────────────────────────────────────────────────
// EffectRegistry.h — Tabla única de efectos (reemplaza switch + effectNames[])
// ──────────────────────────────────────────────────────────────────────────────
// Cada efecto se registra con su nombre JSON (para el frontend) y un puntero
// al método StripLed::run*() que lo ejecuta. El orden en la tabla define el
// effectId (index 0 = ID 1).
//
// Uso:
//   runEffectById(effectId);       // ejecuta el efecto
//   getEffectJsonName(effectId);   // nombre para el JSON del WebSocket
//
// Para agregar un efecto nuevo: añadir línea en effectRegistry[] (y el botón
// en el frontend). El switch de 20 cases ya no existe.
// ──────────────────────────────────────────────────────────────────────────────
#pragma once

#include <stdint.h>

// ============================================================================
// TIPO: puntero a función miembro de StripLed que ejecuta un efecto
// ============================================================================

using EffectRunner = void (StripLed::*)();

// ============================================================================
// STRUCT: entrada del registro de efectos
// ============================================================================

struct EffectEntry {
    const char* jsonName;     // Clave JSON usada por el frontend (e.g. "fireStatus")
    EffectRunner runner;      // Puntero al método StripLed::run*()
};

// ============================================================================
// EFFECT REGISTRY — fuente única de verdad
// ============================================================================
// El orden debe coincidir con effectMap del frontend (data/js/main.js)
// y con los IDs históricos del switch original.
// ============================================================================

inline const EffectEntry effectRegistry[] = {
    { "fireStatus",             &StripLed::runFire },
    { "movingdotStatus",        &StripLed::runMovingDot },
    { "rainbowbeatStatus",      &StripLed::runRainbowBeat },
    { "rwbStatus",              &StripLed::runRedWhiteBlue },
    { "rippleStatus",           &StripLed::runRipple },
    { "twinkleStatus",          &StripLed::runTwinkle },
    { "ballsStatus",            &StripLed::runBalls },
    { "juggleStatus",           &StripLed::runJuggle },
    { "sinelonStatus",          &StripLed::runSinelon },
    { "cometStatus",            &StripLed::runComet },
    { "breathStatus",           &StripLed::runBreath },
    { "colorSweepStatus",       &StripLed::runColorSweep },
    { "rainbowVUStatus",        &StripLed::runRainbowVU },
    { "oldVUStatus",            &StripLed::runOldVU },
    { "rainbowHueVUStatus",     &StripLed::runRainbowHueVU },
    { "rippleVUStatus",         &StripLed::runRippleVU },
    { "threebarsVUStatus",      &StripLed::runThreebarsVU },
    { "oceanVUStatus",          &StripLed::runOceanVU },
    { "tempNEOStatus",          &StripLed::runTemperature },
    { "battNEOStatus",          &StripLed::runBattery },
};

// ============================================================================
// CONSTANTES
// ============================================================================

inline constexpr uint8_t EFFECT_COUNT =
    sizeof(effectRegistry) / sizeof(effectRegistry[0]);

// ============================================================================
// HELPERS INLINE
// ============================================================================

/// Ejecuta un efecto por su ID (1-based, match del frontend)
inline void runEffectById(uint8_t id) {
    if (id < 1 || id > EFFECT_COUNT) return;
    const EffectEntry& entry = effectRegistry[id - 1];
    (stripLed.*(entry.runner))();
}

/// Retorna el nombre JSON para un effectId
inline const char* getEffectJsonName(uint8_t id) {
    if (id < 1 || id > EFFECT_COUNT) return "unknown";
    return effectRegistry[id - 1].jsonName;
}
