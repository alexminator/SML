# Plan de migración y mejora — Smart Music Lamp
> Documento técnico de refactor progresivo  
> Proyecto: ESP32 + FastLED + FreeRTOS + AsyncWebServer  
> Basado en análisis de: `main.cpp`, `Settings.h`, `Balls.h`, `common.h`, `data.h`, `debug.h`, `Effect.h`

---

## Índice

1. [Resumen ejecutivo](#1-resumen-ejecutivo)
2. [Inventario de problemas actuales](#2-inventario-de-problemas-actuales)
3. [Estructura objetivo](#3-estructura-objetivo)
4. [Fase 1 — Correcciones críticas sin refactor](#fase-1--correcciones-críticas-sin-refactor-~2h)
5. [Fase 2 — Separar configuración](#fase-2--separar-configuración-~1h)
6. [Fase 3 — Estado global centralizado](#fase-3--estado-global-centralizado-~2h)
7. [Fase 4 — EffectRegistry](#fase-4--effectregistry-~3h)
8. [Fase 5 — Partir main.cpp](#fase-5--partir-maincpp-~3h)
9. [Fase 6 — Migrar efectos a Effect base class](#fase-6--migrar-efectos-a-effect-base-class-~4h)
10. [Fase 7 — Parámetros de efectos vía web](#fase-7--parámetros-de-efectos-vía-web-~3h)
11. [Fase 8 — VU meters como VUEffect](#fase-8--vu-meters-como-vueffect-~2h)
12. [Checklist de verificación](#12-checklist-de-verificación)
13. [Referencia rápida de patrones](#13-referencia-rápida-de-patrones)

---

## 1. Resumen ejecutivo

El proyecto compila y funciona correctamente hoy, pero tiene una deuda técnica que dificultará agregar efectos nuevos, modificar parámetros vía web, y mantener el código a futuro. Los problemas no son de lógica sino de organización: definiciones en headers, estado global disperso, y una estructura plana de 35 archivos sin jerarquía.

Este plan migra el proyecto en **8 fases independientes**, cada una compilable y testeable por separado. El tiempo estimado total es **~20 horas** distribuidas a voluntad. Ninguna fase rompe las anteriores.

**Principio rector:** en cada fase, el firmware debe compilar y los efectos deben funcionar antes de pasar a la siguiente.

---

## 2. Inventario de problemas actuales

### Severidad alta — pueden causar comportamiento indefinido

| # | Archivo | Problema | Consecuencia |
|---|---------|----------|-------------|
| A1 | `Settings.h` | Objetos `dht`, `batteryStats` definidos en `.h` | ODR violation si se incluye en múltiples `.cpp` |
| A2 | `Settings.h` | Variables globales definidas en `.h` (`temp`, `hum`, `bt_powerState`, arrays de efectos) | Mismo problema ODR |
| A3 | `data.h` | `const char* WIFI_SSID`, `WIFI_PASS`, `WEB_NAME` definidos en `.h` | Símbolos duplicados |
| A4 | `main.cpp` L892 | `char buffer[bufferSize]` — VLA no estándar en C++ | Comportamiento indefinido en algunos compiladores |

### Severidad media — desperdicio de recursos o fragilidad

| # | Archivo | Problema | Consecuencia |
|---|---------|----------|-------------|
| M1 | `main.cpp` L189-287 | Efecto instanciado nuevo cada frame (`Balls balls = Balls()`) | Constructor+destructor cada ~20ms, estado de animación se reinicia |
| M2 | `main.cpp` L289-378 | `clear()` + `FastLED.show()` antes de efectos que ya limpian sus LEDs | Flash visual innecesario en algunos efectos |
| M3 | `common.h` | `uint8_t peakLeft` definido en `.h` sin `static` | ODR si se incluye en más de una unidad |
| M4 | `common.h` | `wheel()`, `addGlitter()` sin `inline` | Múltiples definiciones si se incluye en más de una unidad |
| M5 | `debug.h` | `debugStr[]`, `traceStampRequired`, `foundNL` sin `static` | Variables compartidas entre unidades de compilación |
| M6 | `debug.h` | `debugNothing()` sin `inline` | Definición duplicada |
| M7 | `main.cpp` L620 | `while(1)` infinito si mDNS falla | Dispositivo congelado permanentemente |
| M8 | `Balls.h` | Usa `vImpact0` que solo existe en `main.cpp` | Dependencia oculta, rompe si se mueve el archivo |

### Severidad baja — calidad y mantenibilidad

| # | Problema |
|---|----------|
| L1 | 35 archivos en un solo directorio sin jerarquía |
| L2 | `main.cpp` de 1788 líneas con responsabilidades mezcladas |
| L3 | Switch de 20 cases para efectos — agregar un efecto requiere modificar 3 lugares |
| L4 | Credenciales WiFi potencialmente en repositorio |
| L5 | Parámetros de efectos como variables globales sueltas, difíciles de serializar a JSON |
| L6 | `TaskBatteryMonitor` hace demasiado: monitoreo, detección de fuente de poder, power mgmt y WebSocket tracking |

---

## 3. Estructura objetivo

```
smart-music-lamp/
├─ .gitignore                     ← incluye src/config/secrets.h
├─ platformio.ini
├─ data/                          ← LittleFS web assets, sin cambios
└─ src/
   ├─ main.cpp                    ← ~60 líneas: setup() + loop()
   │
   ├─ config/
   │   ├─ pins.h                  ← solo #define de pines GPIO
   │   ├─ config.h                ← N_PIXELS, VOLTS, thresholds, timings
   │   ├─ network.h               ← extern WIFI_SSID, WIFI_PASS, WEB_NAME
   │   ├─ network.cpp             ← definiciones reales
   │   └─ secrets.h               ← credenciales reales (.gitignore)
   │
   ├─ state/
   │   ├─ AppState.h              ← extern de todos los objetos globales
   │   └─ AppState.cpp            ← definición única: stripLed, batt, leds[], dht…
   │
   ├─ debug/
   │   └─ debug.h                 ← corregido: static vars, inline funcs
   │
   ├─ effects/
   │   ├─ Effect.h                ← base class (ya existe ✓)
   │   ├─ EffectRegistry.h        ← tabla ID→instancia, reemplaza el switch
   │   ├─ effects.h               ← include único de todos los efectos
   │   ├─ utils.h                 ← wheel(), addGlitter() inline (desde common.h)
   │   ├─ Balls.h … Twinkle.h    ← 22 efectos heredando Effect
   │   └─ [Battery.h, Temp.h]    ← efectos visuales también aquí
   │
   ├─ vu/
   │   ├─ VUEffect.h              ← base VU hereda Effect
   │   ├─ RainbowVU.h … OceanVU.h
   │   └─ vu.h                    ← include único de todos los VUs
   │
   ├─ net/
   │   ├─ WebSocket.h / .cpp      ← onWsEvent, notifyClients, handleMessage
   │   └─ WebServer.h / .cpp      ← initWebServer, rutas, initWiFi
   │
   └─ tasks/
       ├─ TaskLED.h
       ├─ TaskBattery.h
       ├─ TaskSensor.h
       ├─ TaskWiFi.h
       ├─ TaskWebSocket.h
       ├─ TaskOnboardLED.h
       └─ tasks.h                 ← include único + TaskHandle_t extern
```

---

## Fase 1 — Correcciones críticas sin refactor (~2h)

> **Objetivo:** eliminar los bugs A1–A4 y M3–M6 sin mover ningún archivo.  
> **Verificación:** el proyecto compila igual que antes.

### 1.1 Corregir `debug.h`

Agregar `static` a las variables globales e `inline` a la función:

```cpp
// ANTES
void debugNothing(...) {}
char debugStr[128];
bool traceStampRequired = true;
bool foundNL = false;

// DESPUÉS
inline void debugNothing(...) {}
static char debugStr[128];
static bool traceStampRequired = true;
static bool foundNL = false;
```

**Por qué `static`:** con `static` en un `.h`, cada unidad de compilación que incluya el header obtiene su **propia copia** de la variable en lugar de competir por una definición global. Para variables de debug esto es exactamente lo que se quiere.

### 1.2 Corregir `common.h`

```cpp
// ANTES
uint8_t peakLeft;
CRGB wheel(byte WheelPos) { ... }
void addGlitter(fract8 chanceOfGlitter) { ... }

// DESPUÉS
static uint8_t peakLeft = 0;          // copia por unidad de compilación
inline CRGB wheel(byte WheelPos) { ... }
inline void addGlitter(fract8 chanceOfGlitter) { ... }
```

### 1.3 Corregir `data.h`

Separar declaración de definición:

```cpp
// data.h — solo declaraciones
#pragma once
#ifndef DEFAULT_WIFI_SSID
#define DEFAULT_WIFI_SSID ""
#endif
#ifndef DEFAULT_WIFI_PASS
#define DEFAULT_WIFI_PASS ""
#endif

extern const char* WIFI_SSID;
extern const char* WIFI_PASS;
extern const char* WEB_NAME;
```

Agregar al inicio de `main.cpp` (antes del `#include "data.h"`):

```cpp
// Definiciones reales — una sola vez
const char* WIFI_SSID = DEFAULT_WIFI_SSID;
const char* WIFI_PASS = DEFAULT_WIFI_PASS;
const char* WEB_NAME  = "sml";
```

### 1.4 Corregir VLA en `main.cpp` (línea ~892)

```cpp
// ANTES — VLA no estándar
const size_t bufferSize = requiredSize + safetyMargin;
char buffer[bufferSize];

// DESPUÉS — tamaño fijo conservador o heap
// Opción A: tamaño fijo (recomendada en ESP32 para evitar fragmentación)
char buffer[1024];
if (bufferSize > sizeof(buffer)) {
    xSemaphoreGive(dataMutex);
    return;
}

// Opción B: heap (si el JSON puede crecer)
char* buffer = (char*)malloc(bufferSize);
if (!buffer) { xSemaphoreGive(dataMutex); return; }
// ... usar buffer ...
free(buffer);
```

### 1.5 Corregir mDNS — eliminar bucle infinito (`main.cpp` ~línea 620)

```cpp
// ANTES
if (!MDNS.begin(WEB_NAME)) {
    debuglnD("Error configurando mDNS!");
    while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
}

// DESPUÉS
if (!MDNS.begin(WEB_NAME)) {
    debuglnE("mDNS failed — web name unavailable, IP access still works");
    // continuar sin mDNS
} else {
    debuglnD("mDNS configurado");
}
```

### 1.6 Corregir instancias por frame en `StripLed::run*()`

Cambiar todas las funciones que crean una instancia nueva cada frame. Patrón a aplicar en los 20 métodos:

```cpp
// ANTES
void StripLed::runBalls() {
    Balls balls = Balls();    // nuevo objeto cada ~20ms
    balls.runPattern();
}

// DESPUÉS
void StripLed::runBalls() {
    static Balls balls;       // se construye UNA sola vez, estado persiste
    balls.runPattern();
}
```

Esto también corrige que el estado de animación de `Balls` (posiciones, velocidades) se preserva entre frames.

### 1.7 Declarar `vImpact0` en `Settings.h`

```cpp
// Agregar en Settings.h junto a las otras variables de Balls
extern float vImpact0;   // definido en main.cpp
```

---

## Fase 2 — Separar configuración (~1h)

> **Objetivo:** crear `config/` con tres archivos especializados.  
> **Verificación:** compilar cambiando solo los paths de include en `Settings.h` y `main.cpp`.

### 2.1 Crear `src/config/pins.h`

Extraer de `Settings.h` todos los `#define` de pines:

```cpp
#pragma once

// LED Strip
#define STRIP_PIN       4

// Onboard LED
#define ONBOARD_LED_PIN LED_BUILTIN

// DHT Sensor
#define DHTPIN          23

// Bluetooth relay
#define SWITCH_PIN      18

// Bluetooth button emulation (MOSFETs)
#define VOLUMENUP_PIN   5
#define VOLUMENDOWN_PIN 19
#define PLAY_PIN        21

// Lamp relay
#define LAMP_PIN        32

// Battery monitoring (TP4056)
#define CHARGE_PIN      34
#define FULL_CHARGE_PIN 35
#define ADC_PIN         33
```

### 2.2 Crear `src/config/config.h`

Extraer de `Settings.h` todas las constantes de comportamiento:

```cpp
#pragma once
#include "pins.h"

// LED Strip
#define N_PIXELS        24
#define VOLTS           5
#define MAX_MILLIAMPS   500
#define COLOR_ORDER     GRB
#define LED_TYPE        WS2812B

// DHT
#define DHTTYPE         DHT22

// Battery monitoring
#define CONV_FACTOR     1.702
#define READS           30
#define MAXV            4.00
#define MINV            3.20
#define BATT_THRESHOLD  30
#define MAX_READS       10
#define FULL_READS      10

// Audio VU
#define AUDIO_IN_PIN    36
#define DC_OFFSET       0
#define NOISE           30
#define SAMPLES         60
#define TOP             (N_PIXELS + 2)
#define PEAK_FALL       20
#define N_PIXELS_HALF   (N_PIXELS / 2)
#define BIAS            1850

// Timing constants
constexpr uint32_t STACK_WARNING_THRESHOLD  = 256;
constexpr uint32_t STACK_CRITICAL_THRESHOLD = 128;
constexpr int      WIFI_MAX_ATTEMPTS        = 40;
constexpr unsigned long WIFI_RETRY_DELAY    = 500;
constexpr unsigned long WIFI_MONITOR_INTERVAL = 2000;
constexpr unsigned long LED_ERROR_FLASH_CYCLE = 200;
constexpr unsigned long LED_ERROR_FLASH_ON    = 50;
constexpr unsigned long BATTERY_CHECK_INTERVAL = 3000;
constexpr unsigned long LITTLEFS_TIMEOUT      = 30000;
constexpr unsigned long WEBSOCKET_UPDATE_INTERVAL = 1000;
constexpr uint8_t  WEBSOCKET_STACK_CHECK_CYCLES   = 10;
constexpr unsigned long SENSOR_CHECK_INTERVAL = 5000;

// Bluetooth button delays
constexpr unsigned long long_delay  = 1000;
constexpr unsigned long short_delay = 200;

// Power management
constexpr unsigned long SLEEP_DURATION        = 60000;
constexpr unsigned long AWAKE_DURATION        = 10000;
constexpr unsigned long POWER_CHANGE_DEBOUNCE = 3000;
constexpr unsigned long WS_WAIT_DURATION      = 30000;
constexpr int BATTERY_CRITICAL_LEVEL          = 15;

// Balls effect physics
#define GRAVITY  -1
#define h0        1
#define NUM_BALLS 3
```

### 2.3 Crear `src/config/secrets.h` + actualizar `.gitignore`

```cpp
// src/config/secrets.h — NUNCA subir a git
#pragma once
#define DEFAULT_WIFI_SSID "tu_red"
#define DEFAULT_WIFI_PASS "tu_password"
```

Agregar al `.gitignore`:
```
src/config/secrets.h
```

En `platformio.ini` agregar un fallback para CI/CD:
```ini
build_flags =
    -D DEFAULT_WIFI_SSID=\"\"
    -D DEFAULT_WIFI_PASS=\"\"
```

### 2.4 Actualizar includes

En `Settings.h`, reemplazar todas las definiciones de pines y constantes con:
```cpp
#include "config/pins.h"
#include "config/config.h"
```

En `main.cpp`, antes de `#include "Settings.h"`:
```cpp
#include "config/secrets.h"   // primero, define DEFAULT_WIFI_SSID/PASS
```

---

## Fase 3 — Estado global centralizado (~2h)

> **Objetivo:** eliminar el ODR de `Settings.h` moviendo definiciones a un `.cpp`.  
> **Verificación:** compilar; en especial verificar que los objetos `dht`, `batt`, `stripLed` funcionan igual.

### 3.1 Crear `src/state/AppState.h`

Solo `extern`, nunca definiciones:

```cpp
#pragma once
#include <FastLED.h>
#include <DHT_U.h>
#include <Battery18650Stats.h>
#include "config/config.h"

// ── Structs (definiciones de tipos, esto sí va en .h) ──────────────────────
struct Led {
    uint8_t pin;
    bool on;
    void update() { digitalWrite(pin, on ? HIGH : LOW); }
};

struct Battery {
    double battVolts;
    int    battLvl;
    bool   fullBatt;
    bool   chargeState;
    Battery() : battVolts(0), battLvl(0), fullBatt(false), chargeState(false) {}
    void battMonitor();
};

struct StripLed {
    int R, G, B;
    int brightness;
    int effectId;
    bool powerState;
    StripLed();
    void simpleColor(int r, int g, int b, int brightness);
    void update();
    void clear();
    // run* methods...
};

// ── Enums ───────────────────────────────────────────────────────────────────
enum PowerState {
    POWER_AC_MODE,
    POWER_BATTERY_ACTIVE,
    POWER_BATTERY_SLEEP,
    POWER_BATTERY_CONNECTING
};

// ── Extern declarations ─────────────────────────────────────────────────────
extern CRGB        leds[N_PIXELS];
extern StripLed    stripLed;
extern Led         onboard_led;
extern Battery     batt;
extern DHT_Unified dht;
extern Battery18650Stats batteryStats;

extern float  temp, hum;
extern bool   bt_powerState;
extern bool   lampState;
extern int    lvlCharge;
extern char   savedSSID[33];
extern char   savedPass[65];
extern float  vImpact0;
extern uint8_t myhue;

// Balls effect state
extern float   h[NUM_BALLS];
extern float   vImpact[NUM_BALLS];
extern float   tCycle[NUM_BALLS];
extern int     pos[NUM_BALLS];
extern long    tLast[NUM_BALLS];
extern float   COR[NUM_BALLS];
extern uint8_t ballsCount;
extern bool    ballsRandomColors;

// Power management state
extern PowerState  currentPowerState;
extern PowerState  previousPowerState;
extern unsigned long lastStateChange;
extern unsigned long sleepCycleStart;
extern bool webSocketClientConnected;
extern bool onBatteryPower;
extern bool powerManagementControllingWiFi;

// Effect params (uno por efecto — se expanden en Fase 7)
extern uint8_t fireCooling, fireSparking;
extern bool    fireReverse;
extern uint8_t sinelonBeat, sinelonFade;
extern uint8_t twinkleSpeed, twinkleIntensity;
extern bool    twinkleRedCool;
// ... resto de params de efectos

// RTOS
extern SemaphoreHandle_t dataMutex;
extern SemaphoreHandle_t wifiMutex;
```

### 3.2 Crear `src/state/AppState.cpp`

```cpp
#include "AppState.h"
#include <DHT_U.h>
#include <Battery18650Stats.h>

// ── Objetos globales ────────────────────────────────────────────────────────
CRGB        leds[N_PIXELS];
StripLed    stripLed;
Led         onboard_led = { ONBOARD_LED_PIN, false };
Battery     batt;
DHT_Unified dht(DHTPIN, DHTTYPE);
Battery18650Stats batteryStats(ADC_PIN, CONV_FACTOR, READS, MAXV, MINV);

// ── Variables de estado ─────────────────────────────────────────────────────
float  temp = 0.0f, hum = 0.0f;
bool   bt_powerState  = false;
bool   lampState      = false;
int    lvlCharge      = 0;
char   savedSSID[33]  = {0};
char   savedPass[65]  = {0};
float  vImpact0       = sqrt(-2 * GRAVITY * h0);
uint8_t myhue         = 0;

// Balls
float   h[NUM_BALLS]       = {0};
float   vImpact[NUM_BALLS] = {0};
float   tCycle[NUM_BALLS]  = {0};
int     pos[NUM_BALLS]     = {0};
long    tLast[NUM_BALLS]   = {0};
float   COR[NUM_BALLS]     = {0};
uint8_t ballsCount         = 3;
bool    ballsRandomColors  = false;

// Power management
PowerState  currentPowerState  = POWER_AC_MODE;
PowerState  previousPowerState = POWER_AC_MODE;
unsigned long lastStateChange  = 0;
unsigned long sleepCycleStart  = 0;
bool webSocketClientConnected  = false;
bool onBatteryPower            = false;
bool powerManagementControllingWiFi = false;

// Effect params
uint8_t fireCooling = 55, fireSparking = 50;
bool    fireReverse = false;
// ... resto

// RTOS
SemaphoreHandle_t dataMutex = NULL;
SemaphoreHandle_t wifiMutex = NULL;
```

### 3.3 Eliminar definiciones de `Settings.h`

En `Settings.h`, borrar todas las definiciones de objetos y variables (las que no son `struct`, `enum`, o `#define`). Reemplazar con `#include "state/AppState.h"` al final del archivo.

---

## Fase 4 — EffectRegistry (~3h)

> **Objetivo:** eliminar el `switch` de 20 cases y el bug de instancias por frame.  
> **Verificación:** todos los efectos deben funcionar igual, ahora con estado persistente.

### 4.1 Crear `src/effects/EffectRegistry.h`

```cpp
#pragma once
#include "Effect.h"
#include "effects.h"          // include de todos los efectos
#include "../state/AppState.h"

struct EffectEntry {
    const char* name;         // nombre para el JSON del WebSocket
    Effect*     instance;
};

// Tabla de efectos — orden = effectId usado por el WebSocket
// effectId 0 = color sólido (manejado por simpleColor, no entra aquí)
inline EffectEntry effectRegistry[] = {
    { "fire",           new Fire(leds, N_PIXELS)        },  // id 1
    { "movingDot",      new MovingDot(leds, N_PIXELS)   },  // id 2
    { "rainbowBeat",    new RainbowBeat(leds, N_PIXELS) },  // id 3
    { "redWhiteBlue",   new RedWhiteBlue(leds, N_PIXELS)},  // id 4
    { "ripple",         new Ripple(leds, N_PIXELS)      },  // id 5
    { "twinkle",        new Twinkle(leds, N_PIXELS)     },  // id 6
    { "balls",          new Balls(leds, N_PIXELS)       },  // id 7
    { "juggle",         new Juggle(leds, N_PIXELS)      },  // id 8
    { "sinelon",        new Sinelon(leds, N_PIXELS)     },  // id 9
    { "comet",          new Comet(leds, N_PIXELS)       },  // id 10
    { "breath",         new Breath(leds, N_PIXELS)      },  // id 11
    { "colorSweep",     new ColorSweep(leds, N_PIXELS)  },  // id 12
    { "rainbowVU",      new RainbowVU(leds, N_PIXELS)   },  // id 13
    { "oldVU",          new OldskoolVU(leds, N_PIXELS)  },  // id 14
    { "rainbowHueVU",   new RainbowHueVU(leds, N_PIXELS)},  // id 15
    { "rippleVU",       new RippleVU(leds, N_PIXELS)    },  // id 16
    { "threebarsVU",    new ThreebarsVU(leds, N_PIXELS) },  // id 17
    { "oceanVU",        new OceanVU(leds, N_PIXELS)     },  // id 18
    { "temperature",    new Temperature(leds, N_PIXELS) },  // id 19
    { "battery",        new Charge(leds, N_PIXELS)      },  // id 20
};

constexpr uint8_t EFFECT_COUNT =
    sizeof(effectRegistry) / sizeof(effectRegistry[0]);

// Obtener nombre de efecto por ID (para el JSON)
inline const char* getEffectName(uint8_t id) {
    if (id == 0) return "solidColor";
    if (id <= EFFECT_COUNT) return effectRegistry[id - 1].name;
    return "unknown";
}

// Obtener instancia por ID
inline Effect* getEffect(uint8_t id) {
    if (id < 1 || id > EFFECT_COUNT) return nullptr;
    return effectRegistry[id - 1].instance;
}
```

### 4.2 Reemplazar `StripLed::update()`

```cpp
// En main.cpp (o en el futuro en state/AppState.cpp)
void StripLed::update() {
    if (!powerState) {
        clear();
        return;
    }
    if (effectId == 0) {
        simpleColor(R, G, B, brightness);
        return;
    }
    Effect* fx = getEffect(effectId);
    if (fx) fx->run();
}
```

El switch de 20 cases desaparece completamente. Agregar un nuevo efecto ahora es:
1. Crear el `.h` en `effects/`
2. Agregarlo a `effects.h`
3. Agregar una línea en `effectRegistry[]`

### 4.3 Crear `src/effects/effects.h`

Include único de todos los efectos:

```cpp
#pragma once
// Efectos visuales
#include "Balls.h"
#include "BouncingBalls.h"
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
// Indicadores
#include "Battery.h"
#include "Temp.h"
// VU
#include "../vu/vu.h"
```

---

## Fase 5 — Partir `main.cpp` (~3h)

> **Objetivo:** reducir `main.cpp` de 1788 a ~60 líneas extrayendo módulos.  
> **Orden:** extraer de menor a mayor dependencia.

### 5.1 Extraer `src/tasks/tasks.h`

Mover todas las funciones `Task*` y sus `TaskHandle_t`:

```cpp
// tasks/tasks.h
#pragma once
#include <Arduino.h>
#include "../state/AppState.h"

// Task handles
extern TaskHandle_t TaskWebSocketHandle;
extern TaskHandle_t TaskBatteryMonitorHandle;
extern TaskHandle_t TaskLEDControlHandle;
extern TaskHandle_t TaskWiFiMonitorHandle;
extern TaskHandle_t TaskSensorHandle;
extern TaskHandle_t TaskOnboardLEDHandle;

// Task functions
void TaskWebSocket(void* pvParameters);
void TaskBatteryMonitor(void* pvParameters);
void TaskLEDControl(void* pvParameters);
void TaskWiFiMonitor(void* pvParameters);
void TaskSensor(void* pvParameters);
void TaskOnboardLED(void* pvParameters);

// Helper
void initTasks();
```

```cpp
// tasks/tasks.cpp
#include "tasks.h"
#include "../net/WebSocket.h"

TaskHandle_t TaskWebSocketHandle    = NULL;
TaskHandle_t TaskBatteryMonitorHandle = NULL;
TaskHandle_t TaskLEDControlHandle   = NULL;
TaskHandle_t TaskWiFiMonitorHandle  = NULL;
TaskHandle_t TaskSensorHandle       = NULL;
TaskHandle_t TaskOnboardLEDHandle   = NULL;

void TaskWebSocket(void* pvParameters) {
    // ... código actual de main.cpp ...
}
// ... resto de tasks ...

void initTasks() {
    xTaskCreatePinnedToCore(TaskWebSocket,    "WebSocketTask",     4096, NULL, 1, &TaskWebSocketHandle,     0);
    xTaskCreatePinnedToCore(TaskBatteryMonitor,"BatteryMonitorTask",2048, NULL, 1, &TaskBatteryMonitorHandle, 1);
    xTaskCreatePinnedToCore(TaskLEDControl,   "LEDControlTask",    2048, NULL, 1, &TaskLEDControlHandle,    0);
    xTaskCreatePinnedToCore(TaskWiFiMonitor,  "WiFiMonitorTask",   2048, NULL, 1, &TaskWiFiMonitorHandle,   1);
    xTaskCreatePinnedToCore(TaskSensor,       "SensorTask",        2048, NULL, 1, &TaskSensorHandle,        0);
    xTaskCreatePinnedToCore(TaskOnboardLED,   "LEDOnboardTask",    2048, NULL, 1, &TaskOnboardLEDHandle,    1);
}
```

### 5.2 Extraer `src/net/WebSocket.h/.cpp`

Mover: `onWsEvent`, `handleWebSocketMessage`, `notifyClients`, `bars()`, `initWebSocket()`.

```cpp
// net/WebSocket.h
#pragma once
#include <ESPAsyncWebServer.h>

extern AsyncWebSocket ws;

void initWebSocket();
void notifyClients();
void onWsEvent(AsyncWebSocket*, AsyncWebSocketClient*, AwsEventType, void*, uint8_t*, size_t);
```

### 5.3 Extraer `src/net/WebServer.h/.cpp`

Mover: `initWebServer`, `onRootRequest`, `initWiFi`, `initLittleFS`, el enum `Status`, `processor()`.

### 5.4 Extraer `src/power/PowerMgr.h/.cpp`

Mover toda la máquina de estados de power management:
`transitionToState`, `applyStateConfiguration`, `onPowerSourceChanged`, `onCriticalBatteryLevel`, `handleBatteryConnectingState`, `handleBatterySleepState`, `updatePowerStateMachine`, `checkWebSocketClients`.

### 5.5 `main.cpp` resultante

```cpp
#include <Arduino.h>
#include "config/secrets.h"
#include "config/config.h"
#include "state/AppState.h"
#include "net/WebServer.h"
#include "net/WebSocket.h"
#include "tasks/tasks.h"
#include "power/PowerMgr.h"
#include "debug/debug.h"

#define DEBUGLEVEL DEBUGLEVEL_DEBUGGING
#define DEBUG_SYSTEM
#define DEBUG_WIFI

void setup() {
    delay(3000);

    // GPIO init
    pinMode(onboard_led.pin, OUTPUT);
    pinMode(STRIP_PIN,       OUTPUT);
    pinMode(LAMP_PIN,        OUTPUT);
    pinMode(SWITCH_PIN,      OUTPUT);
    pinMode(CHARGE_PIN,      INPUT);
    pinMode(ADC_PIN,         INPUT);
    pinMode(FULL_CHARGE_PIN, INPUT);
    pinMode(VOLUMENUP_PIN,   OUTPUT);
    pinMode(VOLUMENDOWN_PIN, OUTPUT);
    pinMode(PLAY_PIN,        OUTPUT);

    digitalWrite(LAMP_PIN,        HIGH);
    digitalWrite(SWITCH_PIN,      HIGH);
    digitalWrite(VOLUMENUP_PIN,   LOW);
    digitalWrite(VOLUMENDOWN_PIN, LOW);
    digitalWrite(PLAY_PIN,        LOW);

    Serial.begin(115200);
    initMutexes();
    dht.begin();

    FastLED.addLeds<LED_TYPE, STRIP_PIN, COLOR_ORDER>(leds, N_PIXELS)
           .setCorrection(TypicalLEDStrip);
    FastLED.setMaxPowerInVoltsAndMilliamps(VOLTS, MAX_MILLIAMPS);
    FastLED.setBrightness(stripLed.brightness);
    FastLED.clear();
    FastLED.show();

    // Init Balls effect state
    for (int i = 0; i < NUM_BALLS; i++) {
        tLast[i]   = millis();
        h[i]       = h0;
        pos[i]     = 0;
        vImpact[i] = vImpact0;
        tCycle[i]  = 0;
        COR[i]     = 0.90f - float(i) / pow(NUM_BALLS, 2);
    }

    initLittleFS();
    initWiFi();
    initWebSocket();
    initWebServer();
    initTasks();
}

void loop() {
    // Todo corre en FreeRTOS tasks
}
```

---

## Fase 6 — Migrar efectos a `Effect` base class (~4h)

> **Objetivo:** cada efecto hereda `Effect`, recibe `leds*` y `numLeds`, lee parámetros de `params`.  
> **Estrategia:** migrar uno por uno, empezando por el más simple (`Breath`) y terminando con el más complejo (`Balls`).

### 6.1 Patrón de migración por efecto

Usando `Balls` como ejemplo completo:

```cpp
// ANTES — Balls.h
#include "Settings.h"
class Balls {
public:
    Balls(){};
    void runPattern();
};

void Balls::runPattern() {
    for (int i = 0; i < ballsCount; i++) {   // lee global
        // usa h[], vImpact[], pos[], tLast[], COR[], tCycle[]  (todos globales)
        leds[pos[i]] = CHSV(..., stripLed.brightness);  // lee global
    }
    FastLED.show();
}
```

```cpp
// DESPUÉS — effects/Balls.h
#pragma once
#include "Effect.h"

class Balls : public Effect {
    // Estado de animación como miembros (no globales)
    float  _h[NUM_BALLS];
    float  _vImpact[NUM_BALLS];
    float  _tCycle[NUM_BALLS];
    int    _pos[NUM_BALLS];
    long   _tLast[NUM_BALLS];
    float  _COR[NUM_BALLS];

public:
    Balls(CRGB* ledArray, uint16_t numLed) : Effect(ledArray, numLed) {
        // params.custom1 = ballsCount (0-7, default 3)
        // params.check1  = randomColors (default false)
        params.custom1 = 3;
        params.check1  = false;

        float v0 = sqrt(-2.0f * GRAVITY * h0);
        for (int i = 0; i < NUM_BALLS; i++) {
            _tLast[i]   = millis();
            _h[i]       = h0;
            _pos[i]     = 0;
            _vImpact[i] = v0;
            _tCycle[i]  = 0;
            _COR[i]     = 0.90f - float(i) / pow(NUM_BALLS, 2);
        }
    }

    void render() override {
        uint8_t count = params.custom1;    // ballsCount desde params
        bool randomColors = params.check1; // randomColors desde params

        for (int i = 0; i < count; i++) {
            _tCycle[i] = millis() - _tLast[i];
            _h[i] = 0.5f * GRAVITY * pow(_tCycle[i] / 1000.0f, 2.0f)
                    + _vImpact[i] * _tCycle[i] / 1000.0f;

            if (_h[i] < 0) {
                _h[i] = 0;
                _vImpact[i] *= _COR[i];
                _tLast[i] = millis();
                if (_vImpact[i] < 0.01f)
                    _vImpact[i] = sqrt(-2.0f * GRAVITY * h0);
            }
            _pos[i] = round(_h[i] * (numLeds - 1) / h0);
        }

        for (int i = 0; i < count; i++) {
            leds[_pos[i]] = randomColors
                ? CHSV(random8(), 255, params.intensity)
                : CHSV(uint8_t(i * 40), 255, params.intensity);
        }
        FastLED.show();
        for (int i = 0; i < count; i++) leds[_pos[i]] = CRGB::Black;
    }
};
```

### 6.2 Orden de migración recomendado

Empezar por efectos sin estado interno (más fáciles):

1. `Breath` — solo usa `params.speed`
2. `ColorSweep` — usa `params.speed`
3. `RainbowBeat` — usa `params.speed`, `params.custom1`
4. `Sinelon` — usa `params.speed`, `params.custom1`
5. `Juggle` — usa `params.custom1`, `params.speed`, `params.intensity`
6. `Comet` — usa `params.speed`, `params.custom1`, `params.check1`
7. `Fire` — usa `params.custom1` (cooling), `params.custom2` (sparking), `params.check1` (reverse)
8. `Twinkle` — usa `params.speed`, `params.intensity`, `params.check1`
9. `Ripple` — usa `params.custom1` (size), `params.check1` (mirror)
10. `MovingDot` — usa `params.speed`, `params.custom1`
11. `Balls` — más complejo, estado interno de física

### 6.3 Mapeo de parámetros por efecto

| Efecto | `speed` | `intensity` | `custom1` | `custom2` | `custom3` | `check1` | `check2` |
|--------|---------|-------------|-----------|-----------|-----------|----------|----------|
| Fire | — | — | cooling | sparking | — | reverse | — |
| Balls | — | brightness | count | — | — | randomColors | — |
| Sinelon | bpm | — | fade | — | — | — | — |
| Twinkle | speed | intensity | — | — | — | redCool | — |
| Ripple | — | — | size | — | — | mirror | — |
| Comet | speed | — | trail | — | — | blur | — |
| Breath | speed | — | — | — | — | — | — |
| ColorSweep | speed | — | — | — | — | — | — |
| Juggle | speed | intensity | dots | — | — | — | — |
| RainbowBeat | speed | — | delta | — | — | — | — |
| MovingDot | speed | — | fade | — | — | — | — |

---

## Fase 7 — Parámetros de efectos vía web (~3h)

> **Objetivo:** permitir cambiar parámetros de cualquier efecto desde el WebSocket sin tocar código C++.

### 7.1 Protocolo WebSocket — nuevo mensaje `setParams`

Desde el frontend JavaScript:
```json
{
  "action": "setParams",
  "effectId": 7,
  "speed": 128,
  "intensity": 200,
  "custom1": 4,
  "check1": true
}
```

### 7.2 Handler en `net/WebSocket.cpp`

```cpp
else if (strcmp(action, "setParams") == 0) {
    uint8_t id = json["effectId"].as<uint8_t>();
    Effect* fx = getEffect(id);
    if (fx) {
        if (json.containsKey("speed"))     fx->setSpeed(json["speed"].as<uint8_t>());
        if (json.containsKey("intensity")) fx->setIntensity(json["intensity"].as<uint8_t>());
        if (json.containsKey("custom1"))   fx->setCustom1(json["custom1"].as<uint8_t>());
        if (json.containsKey("custom2"))   fx->setCustom2(json["custom2"].as<uint8_t>());
        if (json.containsKey("custom3"))   fx->setCustom3(json["custom3"].as<uint8_t>());
        if (json.containsKey("check1"))    fx->setCheck1(json["check1"].as<bool>());
        if (json.containsKey("check2"))    fx->setCheck2(json["check2"].as<bool>());
        if (json.containsKey("check3"))    fx->setCheck3(json["check3"].as<bool>());
    }
    notifyClients();
}
```

### 7.3 Serializar params en `notifyClients()`

```cpp
// Dentro del JSON de notifyClients()
Effect* currentFx = getEffect(stripLed.effectId);
if (currentFx) {
    JsonObject p = json["params"].to<JsonObject>();
    p["speed"]     = currentFx->getSpeed();
    p["intensity"] = currentFx->getIntensity();
    p["custom1"]   = currentFx->getCustom1();
    p["custom2"]   = currentFx->getCustom2();
    p["check1"]    = currentFx->getCheck1();
    p["check2"]    = currentFx->getCheck2();
}
```

### 7.4 Persistencia de parámetros en LittleFS (opcional, Fase 7b)

```cpp
// Guardar al cambiar params
void saveEffectParams() {
    JsonDocument doc;
    for (uint8_t i = 0; i < EFFECT_COUNT; i++) {
        Effect* fx = effectRegistry[i].instance;
        JsonObject e = doc[effectRegistry[i].name].to<JsonObject>();
        e["speed"]     = fx->getSpeed();
        e["intensity"] = fx->getIntensity();
        e["custom1"]   = fx->getCustom1();
        e["check1"]    = fx->getCheck1();
    }
    File f = LittleFS.open("/params.json", "w");
    serializeJson(doc, f);
    f.close();
}

// Cargar en setup()
void loadEffectParams() {
    if (!LittleFS.exists("/params.json")) return;
    File f = LittleFS.open("/params.json", "r");
    JsonDocument doc;
    if (deserializeJson(doc, f) == DeserializationError::Ok) {
        for (uint8_t i = 0; i < EFFECT_COUNT; i++) {
            const char* name = effectRegistry[i].name;
            if (doc.containsKey(name)) {
                Effect* fx = effectRegistry[i].instance;
                fx->setSpeed(doc[name]["speed"] | DEFAULT_SPEED);
                fx->setIntensity(doc[name]["intensity"] | DEFAULT_INTENSITY);
                fx->setCustom1(doc[name]["custom1"] | DEFAULT_C1);
                fx->setCheck1(doc[name]["check1"] | false);
            }
        }
    }
    f.close();
}
```

---

## Fase 8 — VU meters como `VUEffect` (~2h)

> **Objetivo:** eliminar las variables globales de VU de `common.h` encapsulándolas en la clase base.

### 8.1 Crear `src/vu/VUEffect.h`

```cpp
#pragma once
#include "../effects/Effect.h"
#include "../config/config.h"

class VUEffect : public Effect {
protected:
    // Estado de VU encapsulado — ya no son globales
    int      _vol[SAMPLES]     = {0};
    int      _lvl              = 0;
    int      _minLvlAvg        = 0;
    int      _maxLvlAvg        = 512;
    uint8_t  _volCount         = 0;
    uint8_t  _peak             = 0;

public:
    VUEffect(CRGB* ledArray, uint16_t numLed)
        : Effect(ledArray, numLed) {}

protected:
    uint16_t auxReading() {
        int n = analogRead(AUDIO_IN_PIN);
        n = abs(n - BIAS - DC_OFFSET);
        n = (n <= NOISE) ? 0 : (n - NOISE);
        _lvl = ((_lvl * 7) + n) >> 3;
        _vol[_volCount] = n;
        _volCount = ++_volCount % SAMPLES;
        uint16_t height = TOP * (_lvl - _minLvlAvg)
                         / (long)(_maxLvlAvg - _minLvlAvg);
        return constrain(height, 0, TOP);
    }

    void dropPeak() {
        static uint8_t dotCount = 0;
        if (++dotCount >= PEAK_FALL) {
            if (_peak > 0) _peak--;
            dotCount = 0;
        }
    }

    void averageReadings() {
        uint16_t minLvl = _vol[0], maxLvl = _vol[0];
        for (int i = 1; i < SAMPLES; i++) {
            if (_vol[i] < minLvl) minLvl = _vol[i];
            else if (_vol[i] > maxLvl) maxLvl = _vol[i];
        }
        if ((maxLvl - minLvl) < TOP) maxLvl = minLvl + TOP;
        _minLvlAvg = (_minLvlAvg * 63 + minLvl) >> 6;
        _maxLvlAvg = (_maxLvlAvg * 63 + maxLvl) >> 6;
    }
};
```

Cada `vu*.h` hereda `VUEffect` en lugar de leer variables globales. El archivo `common.h` queda vacío y puede eliminarse, con sus funciones utilidad (`wheel`, `addGlitter`) movidas a `effects/utils.h`.

---

## 12. Checklist de verificación

### Por fase
- [ ] **F1:** `debug.h` tiene `static`/`inline` — proyecto compila sin warnings ODR
- [ ] **F1:** `common.h` tiene `static`/`inline` — ídem
- [ ] **F1:** `data.h` sin definiciones — `main.cpp` define los punteros
- [ ] **F1:** Sin VLA en `notifyClients()` — buffer de tamaño fijo
- [ ] **F1:** mDNS falla gracefully — dispositivo no se congela
- [ ] **F1:** Efectos con `static instance` — estado persiste entre frames
- [ ] **F2:** `config/pins.h` compila solo sin errores
- [ ] **F2:** `secrets.h` en `.gitignore` — verificar con `git status`
- [ ] **F3:** `AppState.cpp` compila — no hay símbolos duplicados en el linker
- [ ] **F3:** Eliminar `extern` duplicados del final de `Settings.h`
- [ ] **F4:** `EffectRegistry` — todos los efectos responden en la UI web
- [ ] **F4:** `switch` de 20 cases eliminado de `update()`
- [ ] **F5:** `main.cpp` < 100 líneas — verificar `wc -l src/main.cpp`
- [ ] **F5:** `TaskBatteryMonitor` simplificado — solo monitoreo de batería
- [ ] **F6:** Cada efecto migrado — verificar visualmente en el hardware
- [ ] **F6:** Variables globales de efectos eliminadas de `AppState`
- [ ] **F7:** `setParams` desde WebSocket — cambiar `count` de Balls en vivo
- [ ] **F7:** `notifyClients` serializa params del efecto activo
- [ ] **F8:** `common.h` eliminado — VU funciona igual que antes

### Final
- [ ] Sin warnings de ODR al compilar (`-Wall`)
- [ ] Heap libre después de setup > 100KB (`ESP.getFreeHeap()`)
- [ ] Stack de cada task > `STACK_WARNING_THRESHOLD` en runtime
- [ ] Todos los efectos funcionan en hardware
- [ ] Parámetros persisten tras reinicio (si se implementó F7b)
- [ ] `.gitignore` protege `secrets.h`

---

## 13. Referencia rápida de patrones

### Agregar un nuevo efecto (post-refactor)

1. Crear `src/effects/MiEfecto.h` heredando `Effect`:
```cpp
#pragma once
#include "Effect.h"
class MiEfecto : public Effect {
public:
    MiEfecto(CRGB* l, uint16_t n) : Effect(l, n) {
        params.speed = 128;  // defaults
    }
    void render() override {
        // lógica usando leds[], numLeds, params.speed, etc.
        FastLED.show();
    }
};
```
2. Agregar `#include "MiEfecto.h"` en `effects/effects.h`
3. Agregar `{ "miEfecto", new MiEfecto(leds, N_PIXELS) }` en `EffectRegistry.h`
4. Agregar el botón en el frontend — el ID es la posición en el array + 1

### Cambiar un parámetro de efecto desde el frontend
```javascript
ws.send(JSON.stringify({
    action: "setParams",
    effectId: 7,       // Balls
    custom1: 5,        // 5 bolas
    check1: true       // colores random
}));
```

### Depurar una task específica
```cpp
// Habilitar solo el módulo que interesa en main.cpp
#define DEBUG_LED          // TaskLEDControl
// #define DEBUG_BATTERY   // TaskBatteryMonitor
// #define DEBUG_WEBSOCKET // TaskWebSocket
```
