# SML — Smart Music Lamp
ESP32 + FastLED + WebSocket web UI. Firmware compilado con PlatformIO.

## Compilación

```
C:\Users\ale\.platformio\penv\Scripts\platformio.exe run
```

Esto compila el firmware para el entorno `esp32doit-devkit-v1` definido en `platformio.ini`.

Ejecutar desde la raíz del proyecto (`D:/Proyectos/SML`), donde está `platformio.ini`. `platformio.exe` vive en el `penv` de tu usuario; si no está en `PATH`, usa el path absoluto de arriba.

## Subida / envío al ESP32

```
C:\Users\ale\.platformio\penv\Scripts\platformio.exe run --target upload
```

Necesita el puerto serial configurado en `platformio.ini` o el flag `-e` con el entorno correcto si usas otro board.

## Construir sin subir

```
C:\Users\ale\.platformio\penv\Scripts\platformio.exe run --target build
```

## Limpiar artefactos

```
C:\Users\ale\.platformio\penv\Scripts\platformio.exe run --target clean
```

## Efectos y efectos VU
Todos los efectos viven en `src/effects/`; el VU meter en `src/vu/`. Los agregadores únicos son:
- `src/effects/effects.h` — incluye único de efectos
- `src/vu/vu.h` — incluye único de VU effects

## Persistencia
- `/params.json` — parámetros de efectos (guardados por el firmware)
- `/battlog.json` — historial de batería

## Dependencias clave
- ArduinoJson @^7.0.0 en `platformio.ini`
  - **Nota importante**: la versión resuelta (v7.4.3) NO tiene
    `JsonDocument::reserve()`. Para documentos con capacidad mínima conocida,
    usa `DynamicJsonDocument doc(capacity);` en vez de
    `JsonDocument doc; doc.reserve(capacity);`. El primero sigue soportado
    en v7 y garantiza la capacidad.
- ESPAsyncWebServer / AsyncTCP
- FastLED
- ElegantOTA
