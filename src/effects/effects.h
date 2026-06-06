// ──────────────────────────────────────────────────────────────────────────────
// effects.h — Include único de todos los efectos
// ──────────────────────────────────────────────────────────────────────────────
// Después de mover todos los .h a src/effects/, este archivo sirve como punto
// único de inclusión. Cualquier .cpp que necesite efectos solo incluye este.
// ──────────────────────────────────────────────────────────────────────────────
#pragma once

// ── Efectos visuales activos ────────────────────────────────────────────────
#include "Balls.h"
#include "Breath.h"
#include "ColorSweep.h"
#include "ColorWipe.h"
#include "Comet.h"
#include "Dissolve.h"
#include "DualScan.h"
#include "Fade.h"
#include "Fire.h"
#include "Fire2012.h"
#include "Juggle.h"
#include "Meteor.h"
#include "MovingDot.h"
#include "RainbowBeat.h"
#include "RedWhiteBlue.h"
#include "Ripple.h"
#include "RunningLights.h"
#include "Sinelon.h"
#include "Sparkle.h"
#include "TheaterChase.h"
#include "Twinkle.h"

// ── Efectos indicadores (visualizan sensores) ───────────────────────────────
#include "Battery.h"
#include "Temp.h"
