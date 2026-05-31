#ifndef SETTINGS_H
#define SETTINGS_H

// ============================================================================
// SETTINGS.H — HUB DE CONFIGURACIÓN
// ============================================================================
// Este archivo ya no define símbolos directamente. Solo reúne los includes
// que antes estaban dispersos, para que los 19+ archivos que lo incluyen
// (efectos, main.cpp, etc.) sigan compilando sin cambios.
//
// Fase 3 completada: structs, variables globales y externs migrados a:
//   - src/config/pins.h       → pines GPIO
//   - src/config/config.h     → constantes de comportamiento
//   - src/state/AppState.h    → structs, enums, extern declarations
//   - src/state/AppState.cpp  → definiciones únicas (fin del ODR)
//
// En fases futuras (4-8), los archivos pueden migrarse a incluir
// directamente state/AppState.h y eliminar la dependencia de Settings.h.
// ============================================================================

#include "config/pins.h"       // Pines GPIO
#include "config/config.h"     // Constantes de comportamiento
#include "state/AppState.h"    // Structs, enums, externs
#include "effects/EffectRegistry.h"  // Tabla de efectos (requiere AppState.h)

#endif // SETTINGS_H
