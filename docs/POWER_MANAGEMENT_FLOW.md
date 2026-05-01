# Power Management System - Flujo de Estados Detallado

Documentación completa del comportamiento del sistema de power management en cada escenario posible.

## Estados del Power Management

### 1. POWER_AC_MODE (Modo AC - Operación Completa)
- **Trigger**: AC power conectado (cargando o batería llena)
- **WiFi**: Siempre ON (conexión continua)
- **CPU**: 240MHz (máximo rendimiento)
- **Neopixel**: ON (todos los efectos disponibles)
- **TaskWiFiMonitor**: ACTIVO (cuenta desconexiones, reinicia después de 5)
- **Consumo estimado**: ~180mA

### 2. POWER_BATTERY_CONNECTING (Modo Batería - Intentando Conexión)
- **Trigger**: AC power perdido, esperando WebSocket client
- **Duración**: 40s máximo (10s WiFi + 30s WebSocket)
- **WiFi**: ON (intentando conexión)
- **CPU**: 240MHz
- **Neopixel**: OFF
- **TaskWiFiMonitor**: BYPASSED
- **Consumo estimado**: ~120mA

### 3. POWER_BATTERY_ACTIVE (Modo Batería - Cliente Activo)
- **Trigger**: WebSocket client conectado en modo batería
- **WiFi**: ON (mantenido para cliente)
- **CPU**: 240MHz
- **Neopixel**: OFF
- **TaskWiFiMonitor**: BYPASSED
- **Consumo estimado**: ~120mA

### 4. POWER_BATTERY_SLEEP (Modo Batería - Ahorro Máximo)
- **Trigger**: Timeout 40s sin WebSocket client
- **Ciclo**: 60s WiFi OFF / 10s WiFi ON (repite indefinidamente)
- **WiFi**: Modem sleep (60s) / Despierta (10s)
- **CPU**: 80MHz (sleep) / 240MHz (awake)
- **Neopixel**: OFF
- **TaskWiFiMonitor**: BYPASSED
- **Consumo estimado**: ~25mA promedio

## Escenarios Detallados

### Escenario 1: Operación Normal con Energía AC

**Situación**: ESP32 conectado a AC power, web interface abierta

```
ESTADO: POWER_AC_MODE
- WiFi: ON continuo
- CPU: 240MHz
- Neopixel: ON
- TaskWiFiMonitor: ACTIVO
- Web interface: Totalmente funcional
```

### Escenario 2: AC → Cliente Desconecta (Cierro Web SML)

**Resultado**: Estado NO cambia, sigue en POWER_AC_MODE

### Escenario 3: AC → Cliente Desconecta + Router APAGADO

**Resultado**: TaskWiFiMonitor cuenta 5 desconexiones → ESP32 reinicia

### Escenario 4: AC Power Desconectado → Modo Batería (Web Conectada)

**Timeline**:
- T=0s: AC desconectado
- T=3s: Detección de batería (debounce)
- T=3-13s: Verificación WiFi
- T=13-43s: Espera WebSocket
- T≈13s: Transición a BATTERY_ACTIVE

### Escenario 5: Modo Batería + Web Desconecta + Router APAGADO

**Resultado**: Ciclo de 70s (60s sleep + 10s awake) indefinidamente
- TaskWiFiMonitor BYPASSED
- No hay reinicios
- Consumo: ~25mA promedio

## Resumen de Transiciones

| Desde | Evento | Hacia | Condición |
|-------|--------|-------|-----------|
| AC_MODE | AC power perdido | BATTERY_CONNECTING | onBatteryPower = true |
| BATTERY_CONNECTING | Cliente conecta | BATTERY_ACTIVE | webSocketClientConnected = true |
| BATTERY_CONNECTING | Timeout 40s | BATTERY_SLEEP | Sin cliente |
| BATTERY_ACTIVE | Cliente se desconecta | BATTERY_CONNECTING | webSocketClientConnected = false |
| BATTERY_SLEEP | Cliente conecta | BATTERY_ACTIVE | Inmediato (< 1s) |
| CUALQUIERA | AC power restaurado | AC_MODE | onBatteryPower = false |

## Referencias

- **Spec**: docs/superpowers/specs/2026-05-01-power-management-design.md
- **Plan**: docs/superpowers/plans/2026-05-01-power-management.md
- **Código**: src/main.cpp (líneas ~200-1750)
