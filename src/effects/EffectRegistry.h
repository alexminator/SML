// ──────────────────────────────────────────────────────────────────────────────
// EffectRegistry.h — Tabla única de efectos (instancias Effect*)
// ──────────────────────────────────────────────────────────────────────────────
// Cada efecto se registra con su nombre JSON (para el frontend) y una instancia
// Effect* que lo ejecuta. El orden en la tabla define el effectId (index 0 = ID 1).
//
// Uso:
//   runEffectById(effectId);       // ejecuta el efecto via instance->run()
//   getEffectJsonName(effectId);   // nombre para el JSON del WebSocket
//
// Para agregar un efecto nuevo: crear subclase de Effect, añadir línea en
// effectRegistry[] (y el botón en el frontend).
// ──────────────────────────────────────────────────────────────────────────────
#pragma once

#include "Effect.h"

// ============================================================================
// STRUCT: entrada del registro de efectos
// ============================================================================

struct EffectEntry {
    const char* jsonName;     // Clave JSON usada por el frontend (e.g. "fireStatus")
    Effect*     instance;     // Instancia del efecto (puntero a subclase de Effect)
};

// ============================================================================
// EFFECT REGISTRY — fuente única de verdad (definición en EffectRegistry.cpp)
// ============================================================================
// El orden debe coincidir con effectMap del frontend (data/js/main.js)
// y con los IDs históricos del switch original.
// ============================================================================

extern const EffectEntry effectRegistry[];

// ============================================================================
// CONSTANTES
// ============================================================================

extern const uint8_t EFFECT_COUNT;

// ============================================================================
// EFFECT ID CONSTANTS — reemplaza números mágicos en todo el código
// ============================================================================

// Efecto 0 = Solid (color sólido, no usa el registry)
constexpr uint8_t EFFECT_SOLID     = 0;
// Indicadores del sistema (mapean a índices fijos en effectRegistry[])
constexpr uint8_t EFFECT_TEMP      = 18;   // tempNEOStatus
constexpr uint8_t EFFECT_BATTERY   = 19;   // battNEOStatus

// ============================================================================
// HELPERS INLINE
// ============================================================================

/// Ejecuta un efecto por su ID (1-based, match del frontend)
inline void runEffectById(uint8_t id) {
    if (id < 1 || id > EFFECT_COUNT) return;
    const EffectEntry& entry = effectRegistry[id - 1];
    if (entry.instance) entry.instance->run();
}

/// Retorna el nombre JSON para un effectId
inline const char* getEffectJsonName(uint8_t id) {
    if (id < 1 || id > EFFECT_COUNT) return "unknown";
    return effectRegistry[id - 1].jsonName;
}

/// Retorna el puntero a la instancia Effect* para un effectId (1-based)
inline Effect* getEffect(uint8_t id) {
    if (id < 1 || id > EFFECT_COUNT) return nullptr;
    return effectRegistry[id - 1].instance;
}

// ============================================================================
// PERSISTENCIA DE PARÁMETROS (definiciones en EffectRegistry.cpp)
// ============================================================================

/// Guarda todos los params de efectos a LittleFS (/params.json)
void saveEffectParams();

/// Carga todos los params de efectos desde LittleFS (/params.json)
void loadEffectParams();
