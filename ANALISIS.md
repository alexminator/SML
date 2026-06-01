# Smart Music Lamp (SML) — Análisis Completo del Proyecto

> **Smart Music Lamp** es un proyecto IoT basado en **ESP32** que transforma una lámpara convencional en un dispositivo inteligente con control WiFi, altavoz Bluetooth, iluminación RGB con efectos visuales y monitorización ambiental.

---

## 📋 Índice

1. [Resumen General](#-resumen-general)
2. [Hardware y Pines](#-hardware-y-pines)
3. [Estructura del Proyecto](#-estructura-del-proyecto)
4. [Archivos de Configuración (`src/config/`)](#-archivos-de-configuración-srcconfig)
5. [Estado Global (`src/state/`)](#-estado-global-srcstate)
6. [Red y Comunicación (`src/net/`)](#-red-y-comunicación-srcnet)
7. [Gestión de Energía (`src/power/`)](#-gestión-de-energía-srcpower)
8. [Tareas FreeRTOS (`src/tasks/`)](#-tareas-freertos-srctasks)
9. [Efectos LED (`src/effects/`)](#-efectos-led-srceffects)
10. [VU Meters de Audio (`src/vu/`)](#-vu-meters-de-audio-srcvu)
11. [Interfaz Web (`data/`)](#-interfaz-web-data)
12. [Entry Point (`src/main.cpp`)](#-entry-point-srcmaincpp)
13. [Compilación (`platformio.ini`)](#-compilación-platformioini)
14. [Diagrama de Flujo del Sistema](#-diagrama-de-flujo-del-sistema)

---

## 📌 Resumen General

| Característica | Descripción |
|---|---|
| **Microcontrolador** | ESP32 (DOIT DevKit V1) |
| **Framework** | Arduino + FreeRTOS |
| **Conectividad** | WiFi (Station mode) + WebSocket + mDNS |
| **Bluetooth** | Control mediante emulación de pulsaciones (MOSFETs) |
| **Iluminación** | Tira LED WS2812B (24 LEDs) con FastLED |
| **Sensores** | DHT22 (temperatura y humedad) |
| **Batería** | Li-ion con TP4056 (carga), monitorización ADC + Battery18650Stats |
| **Audio** | Entrada analógica (ADC) para visualización VU |
| **Actualización** | OTA vía ElegantOTA |
| **Interfaz** | Web SPA con WebSocket en tiempo real |

---

## 🔌 Hardware y Pines

| Pin ESP32 | Constante | Función |
|---|---|---|
| D4 (4) | `STRIP_PIN` | WS2812B LED strip (data) |
| D5 (5) | `VOLUMENUP_PIN` | Bluetooth Volume Up / Fast Forward |
| D18 (18) | `SWITCH_PIN` | Relé módulo Bluetooth |
| D19 (19) | `VOLUMENDOWN_PIN` | Bluetooth Volume Down / Rewind |
| D21 (21) | `PLAY_PIN` | Bluetooth Play/Pause |
| D23 (23) | `DHTPIN` | DHT22 sensor |
| D32 (32) | `LAMP_PIN` | Relé lámpara principal (IN1) |
| D33 (33) | `ADC_PIN` | Monitorización voltaje batería |
| D34 (34) | `CHARGE_PIN` | TP4056 charging status (active LOW) |
| D35 (35) | `FULL_CHARGE_PIN` | TP4056 full charge status (active LOW) |
| D36 (VP) | `AUDIO_IN_PIN` | Entrada audio para VU meter |

> **Nota:** ADC2 no se puede usar con WiFi activo. Todos los pines ADC2 están libres.

---

## 📁 Estructura del Proyecto

```
SML/
├── src/
│   ├── main.cpp                  # Entry point (setup + loop vacío)
│   ├── config/
│   │   ├── config.h              # Constantes del sistema
│   │   ├── pins.h                # Asignación de pines GPIO
│   │   ├── debug.h               # Macros de logging jerárquico
│   │   ├── debug_config.h        # Configuración unificada de debugging
│   │   └── secrets.h             # Credenciales WiFi (.gitignore)
│   ├── state/
│   │   ├── AppState.h            # Declaraciones extern del estado global
│   │   └── AppState.cpp          # Definiciones únicas + métodos
│   ├── net/
│   │   ├── data.h / data.cpp     # Credenciales WiFi + nombre mDNS
│   │   ├── WebSocket.h/.cpp      # Servidor WebSocket
│   │   └── WebServer.h/.cpp      # Servidor HTTP + WiFi + OTA
│   ├── power/
│   │   └── PowerMgr.h/.cpp       # Máquina de estados de energía
│   ├── tasks/
│   │   └── tasks.h/.cpp          # Tareas FreeRTOS (6 tareas)
│   ├── effects/
│   │   ├── Effect.h              # Clase base para todos los efectos
│   │   ├── EffectRegistry.h/.cpp # Registro de efectos + persistencia
│   │   ├── effects.h             # Include único de todos los efectos
│   │   ├── utils.h               # Funciones auxiliares (wheel, glitter)
│   │   ├── Fire.h, MovingDot.h, RainbowBeat.h, ...
│   │   ├── Battery.h, Temp.h     # Efectos indicadores
│   │   └── BouncingBalls.h, ...  # Efectos futuros (10)
│   └── vu/
│       ├── VUEffect.h            # Clase base VU meter
│       ├── vu.h                  # Include único de VU effects
│       └── vu1.h … vu6.h         # 6 implementaciones VU
├── data/                         # Interfaz web (LittleFS)
│   ├── index.html                # SPA principal
│   ├── css/                      # Estilos (styles, fontawesome, tailwind)
│   ├── js/                       # JavaScript (main, battery, player, date, iro)
│   ├── fonts/                    # Fuentes personalizadas
│   └── img/                      # Iconos SVG
├── lib/
│   └── Battery18650Stats/        # Fork personalizado (voltajes configurables)
└── platformio.ini                # Configuración PlatformIO
```

---

## ⚙️ Archivos de Configuración (`src/config/`)

### `config.h` — Constantes del Sistema

| Sección | Constantes clave | Valores |
|---|---|---|
| **LED Strip** | `N_PIXELS`, `VOLTS`, `MAX_MILLIAMPS`, `COLOR_ORDER`, `LED_TYPE` | 24, 5V, 500mA, GRB, WS2812B |
| **DHT Sensor** | `DHTTYPE` | DHT22 |
| **Batería** | `CONV_FACTOR`, `MAXV`, `MINV`, `BATT_THRESHOLD` | 1.702, 4.00V, 3.20V, 30% |
| **VU Meter** | `NOISE`, `SAMPLES`, `TOP`, `PEAK_FALL`, `BIAS` | 30, 60, 26, 20, 1850 |
| **WiFi** | `WIFI_MAX_ATTEMPTS`, `WIFI_RETRY_DELAY` | 40, 500ms |
| **Power Mgmt** | `SLEEP_DURATION`, `AWAKE_DURATION`, `BATTERY_CRITICAL_LEVEL` | 60s, 10s, 15% |
| **Timings** | `BATTERY_CHECK_INTERVAL`, `SENSOR_CHECK_INTERVAL`, `WEBSOCKET_UPDATE_INTERVAL` | 3s, 5s, 1s |
| **Bluetooth** | `long_delay`, `short_delay` | 1000ms (vol), 200ms (skip) |

### `pins.h` — Asignación de Pines GPIO

Define todas las constantes de pines (`STRIP_PIN`, `DHTPIN`, `LAMP_PIN`, `ADC_PIN`, etc.) mapeando cada función a su GPIO correspondiente.

### `debug.h` — Macros de Logging

Sistema completo de debugging jerárquico con 5 niveles:

| Nivel | Macro | Uso |
|---|---|---|
| `DEBUGLEVEL_NONE` (0) | — | Sin logging |
| `DEBUGLEVEL_ERRORS` (1) | `debugE()` / `debuglnE()` | Errores críticos |
| `DEBUGLEVEL_WARNINGS` (2) | `debugW()` / `debuglnW()` | Advertencias |
| `DEBUGLEVEL_DEBUGGING` (3) | `debugD()` / `debuglnD()` | Debug general |
| `DEBUGLEVEL_VERBOSE` (4) | `debugV()` / `debuglnV()` | Verboso |

**⚠️ Seguridad:** Implementa `debugD_NUM()`, `debugD_FLOAT1()` y `debugD_NUM03()` con conversión manual (itoa) para **evitar el crash de ESP32 con `snprintf()` y format specifiers**.

Cada mensaje incluye timestamp con nivel, nombre de función y línea de código.

### `debug_config.h` — Punto Único de Configuración

Centraliza la selección de nivel de debug y categorías activas:

```cpp
#define DEBUGLEVEL DEBUGLEVEL_DEBUGGING
#define DEBUG_SYSTEM
#define DEBUG_WIFI
#define DEBUG_POWER_MANAGEMENT
// Opcionales: DEBUG_WEBSOCKET, DEBUG_LED, DEBUG_BATTERY, etc.
```

### `secrets.h` — Credenciales WiFi

Contiene `DEFAULT_WIFI_SSID` y `DEFAULT_WIFI_PASS` con valores por defecto. Este archivo está en `.gitignore` — cada desarrollador pone sus credenciales locales.

---

## 🧠 Estado Global (`src/state/`)

### `AppState.h` — Declaraciones

Declara todo el estado global del sistema como `extern`:

**Structs:**
- `Led` — LED onboard con método `update()`
- `Battery` — voltaje, nivel %, flags de carga y batería llena + `battMonitor()`
- `StripLed` — color RGB, brillo, effectId, powerState + `simpleColor()`, `update()`, `clear()`

**Enums:**
- `PowerState` — `POWER_AC_MODE`, `POWER_BATTERY_ACTIVE`, `POWER_BATTERY_SLEEP`, `POWER_BATTERY_CONNECTING`

**Objetos globales:**
- `CRGB leds[24]` — array de LEDs para FastLED
- `stripLed`, `onboard_led`, `batt` — instancias de los structs
- `dht` — sensor DHT_Unified
- `batteryStats` — monitor Battery18650Stats

**Variables de estado:**
- `temp`, `hum` — temperatura y humedad
- `bt_powerState`, `lampState` — estados de Bluetooth y lámpara
- `myhue`, `currentPalette`, `targetPalette` — estado de color
- `savedSSID[33]`, `savedPass[65]` — credenciales persistentes

**Sincronización RTOS:**
- `dataMutex`, `wifiMutex` — semáforos para acceso a datos compartidos
- `initMutexes()` — inicialización

**Estado de energía:**
- `currentPowerState`, `previousPowerState` — máquina de estados
- `webSocketClientConnected`, `onBatteryPower` — flags

### `AppState.cpp` — Definiciones Únicas

Contiene la **única definición** de cada símbolo global (ODR-safe):
- Inicialización de mutexes
- Implementación de `Battery::battMonitor()` — lee ADC, detecta carga/llena
- Implementación de `StripLed::simpleColor()` — pinta todos los LEDs de un color
- Implementación de `StripLed::update()` — según `effectId`, llama al efecto o color sólido

---

## 🌐 Red y Comunicación (`src/net/`)

### `data.h` / `data.cpp` — Credenciales WiFi

```cpp
const char *WIFI_SSID = DEFAULT_WIFI_SSID;  // De build flags o secrets.h
const char *WIFI_PASS = DEFAULT_WIFI_PASS;
const char *WEB_NAME  = "sml";              // mDNS: sml.local
```

### `WebSocket.h` / `WebSocket.cpp` — Servidor WebSocket

**Endpoint:** `ws://<esp32-ip>/ws`

**Outgoing (notifyClients) — cada 1s:**
```json
{
  "bars": "waveStrength-4",        // Señal WiFi
  "battVoltage": 3.85,             // Voltaje batería
  "level": 72,                     // % batería
  "charging": false,               // ¿Cargando?
  "fullbatt": true,                // ¿Batería llena?
  "temperature": 25.3,             // °C
  "humidity": 60.5,                // % HR
  "lampstatus": "on",              // Estado lámpara
  "neostatus": "on",               // Estado neopixel
  "btstatus": "off",               // Estado Bluetooth
  "neobrightness": 130,            // Brillo
  "ssid": "MiWiFi",               // Red conectada
  "ip": "192.168.1.100",          // IP del ESP32
  "rssi": -65,                     // Señal dBm
  "color": { "r": 255, "g": 255, "b": 255 },
  "fireStatus": "on",              // Estado de cada efecto
  "params": { "speed": 128, "intensity": 128, ... }
}
```

**Incoming (handleWebSocketMessage):**

| `action` | Función |
|---|---|
| `toggle` | Enciende/apaga neopixel |
| `lamp` | Enciende/apaga lámpara (relé) |
| `animation` | Activa efecto visual por `effectId` |
| `vu` | Activa efecto VU por `effectId` |
| `indicator` | Activa efecto indicador por `effectId` |
| `slider` | Ajusta brillo (`brightness`) |
| `picker` | Cambia color RGB |
| `music` | Enciende/apaga módulo Bluetooth |
| `volup` / `voldown` | Sube/baja volumen (pulsación larga 1s) |
| `skipL` / `skipR` | Canción anterior/siguiente (pulsación corta 200ms) |
| `play-pause` | Play/Pause (pulsación corta 200ms) |
| `setParams` | Ajusta parámetros del efecto (speed, intensity, custom1-3, check1-3) |

### `WebServer.h` / `WebServer.cpp` — Servidor HTTP

**Funcionalidades:**
- Sirve `index.html` desde LittleFS con sistema de plantillas (`%VAR%`)
- **Rutas:**
  - `GET /` → `index.html` con procesamiento de variables
  - `GET /wifi-info` → JSON con SSID, IP, RSSI
  - `POST /save-wifi` → Guarda credenciales en Preferences + `ESP.restart()`
  - Archivos estáticos desde LittleFS
- **ElegantOTA** en `/update` para actualizaciones OTA
- **mDNS** en `sml.local` (puerto 80)

---

## 🔋 Gestión de Energía (`src/power/`)

### `PowerMgr.h` / `PowerMgr.cpp` — Máquina de Estados

**Arquitectura:** 4 estados con transiciones automáticas:

```
                  ┌──────────────────┐
                  │   POWER_AC_MODE  │ ◄──── AC Power Restored
                  │ (Full operation) │
                  └────────┬─────────┘
                           │ AC Power Lost
                           ▼
              ┌────────────────────────┐
              │ POWER_BATTERY_CONNECTING│
              │ Fase 1: WiFi (10s)     │
              │ Fase 2: Espera WS (30s)│
              └────┬──────────────┬────┘
                   │              │
          Client   │              │ Timeout
          Connects │              ▼
                   │   ┌──────────────────┐
                   │   │ POWER_BATTERY_SLEEP│
                   │   │ 60s WiFi OFF      │
                   │   │ 10s WiFi ON       │
                   │   │ CPU 80MHz         │
                   │   └────────┬─────────┘
                   │            │ WiFi connects
                   ▼            │ in awake window
        ┌──────────────────┐    │
        │POWER_BATTERY_    │◄───┘
        │   ACTIVE         │
        │(Client active)   │
        └────────┬─────────┘
                 │ Client disconnects
                 │ → vuelve a CONNECTING
                 ▼
      ┌──────────────────────┐
      │ POWER_BATTERY_CONNECTING│ (loop)
      └──────────────────────┘
```

**Configuración por estado:**

| Estado | WiFi Sleep | CPU | Neopixel |
|---|---|---|---|
| AC_MODE | OFF | 240MHz | ON |
| BATTERY_ACTIVE | OFF | 240MHz | OFF |
| BATTERY_SLEEP | ON | 80MHz | OFF |
| BATTERY_CONNECTING | OFF | 240MHz | OFF |

**Eventos:**
- `onPowerSourceChanged()` — con debounce de 3s
- `onCriticalBatteryLevel()` — batería < 15%
- `checkWebSocketClients()` — verificación periódica cada 5s

---

## 🎯 Tareas FreeRTOS (`src/tasks/`)

### `tasks.h` / `tasks.cpp` — 6 Tareas Concurrentes

| Tarea | Núcleo | Stack | Prioridad | Intervalo | Función |
|---|---|---|---|---|---|
| `TaskWebSocket` | Core 0 | 4096 | 1 | 1s | Limpia clientes + notifica estado vía WebSocket |
| `TaskBatteryMonitor` | Core 1 | 2048 | 1 | 3s | Lee batería, detecta fuente energía, ejecuta máquina de estados |
| `TaskLEDControl` | Core 0 | 2048 | 1 | 20ms | Renderiza efecto LED activo (50 fps) |
| `TaskWiFiMonitor` | Core 1 | 2048 | 1 | 2s | Reintenta WiFi si es necesario (solo en AC mode) |
| `TaskSensor` | Core 0 | 2048 | 1 | 5s | Lee DHT22 con 3 reintentos |
| `TaskOnboardLED` | Core 1 | 2048 | 1 | 100ms | LED según estado: WiFi+WS → patrón mixto; WiFi solo → 1s blink; nada → OFF |

**Timing del sistema:**
- Efectos LED se actualizan a **50 fps** (cada 20ms)
- WebSocket notifica cada **1 segundo**
- Batería se monitorea cada **3 segundos**
- Sensores se leen cada **5 segundos**
- WiFi se verifica cada **2 segundos**

---

## ✨ Efectos LED (`src/effects/`)

### `Effect.h` — Clase Base

```cpp
class Effect {
protected:
    EffectParams params;    // speed, intensity, custom1-3, check1-3
    CRGB* leds;             // Puntero al array de LEDs
    uint16_t numLeds;       // Número de LEDs
public:
    virtual void render() = 0;  // Cada efecto implementa su lógica visual
    void run();                  // Wrapper que llama a render()
};
```

### `EffectRegistry.h` / `EffectRegistry.cpp` — Registro Central

Mantiene un array de 20 entradas `EffectEntry { jsonName, instance }`. Cada efecto tiene:

- Un **nombre JSON** para el frontend (e.g. `"fireStatus"`)
- Una **instancia** de la subclase correspondiente
- **Persistencia** de parámetros en LittleFS (`/params.json`): `saveEffectParams()` / `loadEffectParams()`

### Efectos Activos (12)

| ID | Clase | Archivo | Descripción |
|---|---|---|---|
| 1 | `FireEffect` | `Fire.h` | Llama realista con cooling (55) y sparking (50) configurables |
| 2 | `MovingDotEffect` | `MovingDot.h` | Punto que se mueve a lo largo de la tira |
| 3 | `RainbowBeatEffect` | `RainbowBeat.h` | Arcoíris continuo |
| 4 | `RedWhiteBlueEffect` | `RedWhiteBlue.h` | Patrón rojo, blanco, azul |
| 5 | `RippleEffect` | `Ripple.h` | Onda expansiva desde el centro |
| 6 | `TwinkleEffect` | `Twinkle.h` | Destellos aleatorios tipo estrellas |
| 7 | `BallsEffect` | `Balls.h` | Simulación física de pelotas rebotando (gravedad, COR) |
| 8 | `JuggleEffect` | `Juggle.h` | Malabarismo con múltiples colores |
| 9 | `SinelonEffect` | `Sinelon.h` | Onda senoidal desplazándose |
| 10 | `CometEffect` | `Comet.h` | Cometa con estela (estela y blur configurables) |
| 11 | `BreathEffect` | `Breath.h` | Efecto de respiración (fade in/out) |
| 12 | `ColorSweepEffect` | `ColorSweep.h` | Barrido de color a través del espectro |

### Efectos Futuros (10 — heredan Effect pero no registrados)

| Archivo | Efecto |
|---|---|
| `BouncingBalls.h` | Pelotas rebotando (variante) |
| `ColorWipe.h` | Barrido de color sólido |
| `Dissolve.h` | Disolución |
| `DualScan.h` | Escaneo dual |
| `Fade.h` | Fundido |
| `Fire2012.h` | Llama versión 2012 |
| `Meteor.h` | Lluvia de meteoros |
| `RunningLights.h` | Luces corredizas |
| `Sparkle.h` | Destellos (variante) |
| `TheaterChase.h` | Persecución tipo teatro |

### Efectos Indicadores (2)

| ID | Clase | Archivo | Descripción |
|---|---|---|---|
| 19 | `TemperatureEffect` | `Temp.h` | Barra de colores: mapea temp 17-40°C a LEDs con degradado |
| 20 | `ChargeEffect` | `Battery.h` | Nivel batería: 0-7% rojo (meteor), 8-14% amarillo, 15-23% verde, >24% azul (olas) |

### `utils.h` — Funciones Auxiliares

- `addGlitter(fract8)` — añade brillos aleatorios blancos
- `wheel(byte)` — convierte hue (0-255) a color RGB (arcoíris)

---

## 🎵 VU Meters de Audio (`src/vu/`)

### `VUEffect.h` — Clase Base

Procesa la entrada de audio del ADC (pin 36) con:

- **Filtro de ruido:** ignora valores ≤ `NOISE` (30)
- **Dampening:** `_lvl = ((_lvl * 7) + n) >> 3`
- **Buffer circular:** 60 muestras para dinámica adaptativa
- **Peak tracking:** decae cada `PEAK_FALL` (20) frames
- **Normalización:** mapea a altura de barra (0-TOP)

### Efectos VU (6)

| ID | Clase | Archivo | Descripción |
|---|---|---|---|
| 13 | `RainbowVUEffect` | `vu1.h` | Arcoíris desde el centro/abajo, verde a púrpura |
| 14 | `OldskoolVUEffect` | `vu2.h` | Estilo clásico de ecualizador |
| 15 | `RainbowHueVUEffect` | `vu3.h` | Arcoíris con tono variable |
| 16 | `RippleVUEffect` | `vu4.h` | Ondas concéntricas con el ritmo |
| 17 | `ThreebarsVUEffect` | `vu5.h` | Tres barras independientes |
| 18 | `OceanVUEffect` | `vu6.h` | Efecto oceánico (hereda Effect directamente) |

---

## 🌐 Interfaz Web (`data/`)

### `index.html` — Single Page Application

**Estructura:**
- **Header:** Logo SML animado (mariposa + texto glowing)
- **Tabs de navegación:**
  1. **Lamp** — Control de lámpara, neopixel, selector de color (iro.js), brillo, 12 efectos visuales
  2. **Music** — Controles Bluetooth (play/pause, volumen, skip, on/off) + 6 efectos VU
  3. **Weather** — Termómetro animado + higrómetro con datos DHT22
  4. **Battery** — Indicador de batería con nivel líquido, burbujas animadas, voltaje

### `js/main.js` — Lógica Principal

- **WebSocket:** conexión persistente, reconexión automática cada 2s
- **Effect Map:** mapeo de 20 botones a IDs de efecto (1-20)
- **Color Picker:** iro.js con sincronización bidireccional ESP32 ↔ navegador
- **Control de brillo:** slider con envío inmediato vía WebSocket
- **Actualización en tiempo real:** temperatura, humedad, batería, señal WiFi
- **WiFi Config:** modal para cambiar credenciales (POST + restart)

### `js/battery.js` — Animación de Batería

- Barra de nivel con altura dinámica y colores degradados
- Burbujas animadas cuando está cargando
- Estados: Full battery, Low battery, Charging, Power in use, In use

### `js/player.js` — Controles de Música

- Botones con eventos pointer para feedback táctil
- Envío de comandos WebSocket: `play-pause`, `skipL`, `skipR`, `volup`, `voldown`

### `js/date.js` — Reloj en Tiempo Real

- Formato 12h con AM/PM
- Día de la semana, fecha, mes y año en español
- Actualización cada 1 segundo

### `css/styles.css` — Estilos

- Diseño **grid responsivo** con variables CSS
- **Fondo animado** con partículas (`circles`)
- **Tabs** con indicador activo
- **Interruptores** estilo iOS con animación
- **Termómetro** con bulbo y columna graduada
- **Batería** con líquido animado y burbujas
- **Color picker** integrado con marco

---

## 🚀 Entry Point (`src/main.cpp`)

```cpp
void setup() {
    // 1. Configuración de pines (relés OFF, MOSFETs LOW)
    pinMode(STRIP_PIN, OUTPUT);      // Neopixel
    pinMode(LAMP_PIN, OUTPUT);       // Relé lámpara
    pinMode(SWITCH_PIN, OUTPUT);     // Relé Bluetooth
    
    // 2. Inicialización de componentes
    Serial.begin(115200);
    initMutexes();                   // Semáforos RTOS
    dht.begin();                     // Sensor DHT22
    
    // 3. Inicialización LED strip
    FastLED.addLeds<WS2812B, 4, GRB>(leds, 24);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);
    FastLED.setBrightness(130);
    
    // 4. Inicialización de subsistemas
    initLittleFS();                  // Sistema de archivos
    loadEffectParams();              // Parámetros de efectos persistentes
    initWiFi();                      // Conexión WiFi
    initWebSocket();                 // Servidor WebSocket
    initWebServer();                 // Servidor HTTP + OTA
    initTasks();                     // 6 tareas FreeRTOS
}

void loop() {
    // Vacío — todo corre en tareas FreeRTOS
}
```

---

## 🔧 Compilación (`platformio.ini`)

| Parámetro | Valor |
|---|---|
| **Plataforma** | `espressif32` |
| **Board** | `esp32doit-devkit-v1` |
| **Framework** | `arduino` |
| **Upload Speed** | 921600 baud |
| **Monitor Speed** | 115200 baud |
| **Filesystem** | LittleFS |
| **Build Flags** | `-DELEGANTOTA_USE_ASYNC_WEBSERVER=1`, credenciales desde build env |

**Dependencias:**
| Librería | Versión | Uso |
|---|---|---|
| `ArduinoJson` | — | Serialización JSON |
| `ESPAsyncWebServer` | ^3.7.3 | Servidor web asíncrono |
| `AsyncTCP` | ^3.3.7 | TCP asíncrono para WebSockets |
| `FastLED` | ^3.5.0 | Control de tira LED |
| `Battery18650Stats` | fork alexminator | Monitor de batería Li-ion |
| `DHT sensor library` | ^1.4.4 | Sensor temp/humedad |
| `Adafruit Unified Sensor` | ^1.1.11 | Driver base de sensores |
| `ElegantOTA` | ^3.1.0 | Actualizaciones OTA |

---

## 🔄 Diagrama de Flujo del Sistema

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          ESP32 BOOT                                     │
│  setup(): pines → mutexes → FastLED → LittleFS → WiFi → WS → HTTP → RTOS│
└─────────────────────────────┬───────────────────────────────────────────┘
                              │
            ┌─────────────────┴──────────────────┐
            │         FREE RTOS KERNEL            │
            │    (6 tareas en 2 núcleos)          │
            └──────┬──────┬──────┬──────┬──────┬──┘
                   │      │      │      │      │
    ┌──────────────┘      │      │      │      └──────────────┐
    ▼                     ▼      ▼      ▼                     ▼
┌─────────┐ ┌──────────┐ ┌────┐ ┌────────┐ ┌──────┐ ┌───────────┐
│WiFi     │ │Battery   │ │LED │ │WebSocket│ │Sensor│ │Onboard LED│
│Monitor  │ │Monitor   │ │Ctrl│ │Notifier │ │DHT22 │ │Indicator  │
│(Core 1) │ │(Core 1)  │ │(C0)│ │(Core 0) │ │(C0)  │ │(Core 1)   │
│2s cycle │ │3s cycle  │20ms │ │1s cycle │5s cycle│100ms cycle │
└─────────┘ └──────────┘ └────┘ └─────────┘ └──────┘ └───────────┘
                 │                                  │
                 ▼                                  ▼
     ┌─────────────────────┐              ┌──────────────────┐
     │  Power Management   │              │  Web Interface   │
     │  State Machine      │◄────────────►│  (Browser SPA)   │
     │  (4 estados)        │  WebSocket   │  + HTTP Server   │
     └─────────────────────┘   JSON       └──────────────────┘
                 │                                  │
                 ▼                                  ▼
     ┌─────────────────────┐              ┌──────────────────┐
     │ HW Control          │              │ User Commands    │
     │ • LED Strip (FastLED)│             │ • Color/Bright   │
     │ • Lamp Relay        │              │ • Effects On/Off │
     │ • Bluetooth MOSFETs │              │ • Music Control  │
     │ • TP4056 Monitoring │              │ • WiFi Config    │
     └─────────────────────┘              │ • OTA Updates    │
                                           └──────────────────┘
```

---

## ⚠️ Notas Técnicas Importantes

### 🚫 Prohibición de `snprintf()`

**NUNCA usar `snprintf()` con format specifiers** (`%d`, `%u`, `%.1f`, etc.) — causa "Interrupt wdt timeout on CPU0" en ESP32. Usar en su lugar:

- `debugD_NUM(val, "%d")` para enteros
- `debugD_FLOAT1(val)` para floats (conversión manual)
- `itoa()` para conversión simple
- Valores numéricos directos en ArduinoJson

### 🔋 Gestión de Batería

- TP4056 con pines `CHARGE_PIN` (active LOW) y `FULL_CHARGE_PIN` (active LOW)
- ADC en pin 33 (ADC1 — seguro con WiFi activo)
- Factor de conversión: 1.702
- Threshold bajo: 30%, crítico: 15%

### 📡 WiFi

- Solo modo Station (no AP)
- Credenciales en Preferences (persistentes tras restart)
- Fallback a credenciales de compilación
- mDNS en `sml.local`
- Puerto HTTP: 80, WebSocket: `/ws`

### 🎵 Audio (VU Meter)

- Entrada analógica por pin D36 (ADC1_VP)
- BIAS: 1850 (ajustable según ADC)
- Filtro de ruido: 30
- Sampling: buffer circular de 60 muestras
- Normalización dinámica automática
