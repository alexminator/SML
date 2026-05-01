# Sistema de Debug Categorizado - Guía de Uso

**Fecha:** 2026-04-16  
**Versión:** 1.0

---

## Resumen

El SML ahora cuenta con un **sistema de debug categorizado** que permite habilitar/deshabilitar mensajes de debug por subsistema individualmente.

---

## Categorías Disponibles

Todas las categorías se definen al principio de `src/main.cpp`:

```cpp
// Sistema general (inicialización, mutex, errores)
#define DEBUG_SYSTEM

// Conexión WiFi y autenticación
#define DEBUG_WIFI

// Servidor web y sistema de archivos LittleFS
#define DEBUG_WEB

// Comunicación WebSocket y JSON
#define DEBUG_WEBSOCKET

// Control de tira LED y efectos
#define DEBUG_LED

// Monitoreo de batería y carga
#define DEBUG_BATTERY

// Sensor de temperatura y humedad (DHT)
#define DEBUG_TEMPERATURE

// Servicios de red (mDNS)
#define DEBUG_NETWORK

// Control Bluetooth (futuro)
// #define DEBUG_BLUETOOTH
```

---

## Cómo Usar

### Habilitar/Deshabilitar Categorías

Para **habilitar** una categoría: descomentar el `#define`

```cpp
#define DEBUG_WIFI        // ✅ Habilitado
#define DEBUG_TEMPERATURE // ✅ Habilitado
// #define DEBUG_LED     // ❌ Deshabilitado
```

Para **deshabilitar** una categoría: comentar el `#define`

```cpp
// #define DEBUG_WIFI   // ❌ Deshabilitado
```

### Ejemplos de Configuración

#### Modo Producción (Sin debug)
```cpp
// Todos los debug deshabilitados
#define DEBUGLEVEL DEBUGLEVEL_NONE
// Todos los #define comentados
```

#### Debug Solo de Batería
```cpp
#define DEBUGLEVEL DEBUGLEVEL_DEBUGGING
#define DEBUG_BATTERY  // Solo mensajes de batería
```

#### Debug Completo
```cpp
#define DEBUGLEVEL DEBUGLEVEL_DEBUGGING
#define DEBUG_SYSTEM
#define DEBUG_WIFI
#define DEBUG_WEB
#define DEBUG_WEBSOCKET
#define DEBUG_LED
#define DEBUG_BATTERY
#define DEBUG_TEMPERATURE
#define DEBUG_NETWORK
```

#### Debug de Desarrollo (Común)
```cpp
#define DEBUGLEVEL DEBUGLEVEL_DEBUGGING
#define DEBUG_WIFI
#define DEBUG_WEBSOCKET
#define DEBUG_BATTERY
// DEBUG_SYSTEM comentado para menos ruido en inicio
// DEBUG_TEMPERATURE comentado (muy verbose)
```

---

## Niveles de Debug

El sistema tiene **4 niveles** de debug (controlados por `DEBUGLEVEL`):

```cpp
DEBUGLEVEL_ERRORS (1)    // Solo errores críticos
DEBUGLEVEL_WARNINGS (2)  // Errores + advertencias
DEBUGLEVEL_DEBUGGING (3) // Errores + advertencias + debug
DEBUGLEVEL_VERBOSE (4)   // Todo lo anterior + verbose
```

### Jerarquía de Funciones

**Nivel ERROR (siempre):**
- `debugE()` / `debuglnE()`

**Nivel WARNING+:**
- `debugW()` / `debuglnW()`

**Nivel DEBUG+:**
- `debugD()` / `debuglnD()`

**Nivel VERBOSE:**
- `debugV()` / `debuglnV()`

---

## Mensajes por Categoría

### DEBUG_SYSTEM (5 mensajes)
- Inicialización de mutexes
- Errores del sistema
- Advertencias de degradación

### DEBUG_WIFI (5 mensajes)
- Conexión WiFi
- Credenciales (guardadas/default)
- Conteo de desconexiones
- Reconexión
- Reinicio por desconexiones

### DEBUG_WEB (3 mensajes)
- Montaje de LittleFS
- Inicio del servidor HTTP
- Errores de interfaz web

### DEBUG_WEBSOCKET (10 mensajes)
- Tamaño de payload JSON
- Errores de deserialización JSON
- Errores de buffer
- Adquisición de mutex WebSocket
- Inicio del servidor WebSocket
- Advertencias de stack

### DEBUG_LED (3 mensajes)
- Estado de lámpara (ON/OFF)
- Cambio de brillo
- Cambio de color RGB

### DEBUG_BATTERY (7 mensajes)
- Estado del cargador
- Estado de pines de carga
- Estado de batería (usándose/cargada/completa)
- Lecturas de voltaje y nivel
- Errores de mutex de batería

### DEBUG_TEMPERATURE (19 mensajes)
- Detalles del sensor de temperatura
- Detalles del sensor de humedad
- Lecturas de temperatura
- Lecturas de humedad
- Errores de lectura con reintentos

### DEBUG_NETWORK (2 mensajes)
- Configuración de mDNS
- Errores de mDNS

---

## Ahorro de Flash

Cada categoría deshabilitada ahorra espacio en flash:

| Categoría | Ahorro aprox. |
|-----------|---------------|
| DEBUG_TEMPERATURE | ~2 KB |
| DEBUG_WEBSOCKET | ~1.5 KB |
| DEBUG_WIFI | ~800 B |
| DEBUG_BATTERY | ~600 B |
| DEBUG_LED | ~400 B |
| DEBUG_WEB | ~300 B |
| DEBUG_NETWORK | ~200 B |
| DEBUG_SYSTEM | ~250 B |

**Total potencial:** ~6 KB de flash ahorrado

---

## Solución de Problemas

### Debug no aparece
1. Verificar que `DEBUGLEVEL` esté configurado correctamente
2. Verificar que la categoría esté descomentada
3. Verificar que el nivel de debug sea suficiente (ej: `debugD()` requiere nivel 3+)

### Error de compilación
1. Verificar que cada `#ifdef` tenga su `#endif` correspondiente
2. Verificar que no haya código lógico dentro de `#ifdef` (como `ESP.restart()`)

### Crash en el ESP32
1. Verificar que no se estén usando concatenaciones de String dentro de `#ifdef`
2. Usar `snprintf` con buffers estáticos
3. Verificar que punteros PROGMEM no se pasen a `snprintf`

---

## Buenas Prácticas

### Para Desarrollo
```cpp
#define DEBUGLEVEL DEBUGLEVEL_DEBUGGING
#define DEBUG_WIFI
#define DEBUG_WEBSOCKET
#define DEBUG_BATTERY
```

### Para Producción
```cpp
#define DEBUGLEVEL DEBUGLEVEL_ERRORS
// Solo errores críticos
```

### Para Troubleshooting Específico
```cpp
#define DEBUGLEVEL DEBUGLEVEL_DEBUGGING
#define DEBUG_TEMPERATURE  // Solo si hay problemas con DHT
```

### Ahorro Máximo de Flash
```cpp
#define DEBUGLEVEL DEBUGLEVEL_NONE
// Todos los #define comentados
```

---

## Referencias

- **Archivo principal:** `src/main.cpp`
- **Sistema de debug:** `src/debug.h`
- **Documentación de bugs:** `docs/superpowers/TESTING_CHECKLIST.md`

---

**FIN DE LA GUÍA**
