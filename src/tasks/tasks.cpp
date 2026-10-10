// ──────────────────────────────────────────────────────────────────────────────
// tasks.cpp — FreeRTOS Task implementations
// ──────────────────────────────────────────────────────────────────────────────
#include "tasks.h"
#include "../state/AppState.h"
#include "net/WebSocket.h"
#include "net/WebServer.h"
#include "power/PowerMgr.h"
#include "config/debug_config.h"
#include <WiFi.h>

// ============================================================================
// Task Handles (single definition)
// ============================================================================

TaskHandle_t TaskWebSocketHandle        = NULL;
TaskHandle_t TaskBatteryMonitorHandle   = NULL;
TaskHandle_t TaskLEDControlHandle       = NULL;
TaskHandle_t TaskWiFiMonitorHandle      = NULL;
TaskHandle_t TaskSensorHandle           = NULL;
TaskHandle_t TaskOnboardLEDHandle       = NULL;

// ============================================================================
// TaskWebSocket — WebSocket cleanup + notify clients
// ============================================================================

void TaskWebSocket(void *pvParameters) {
    UBaseType_t stackHighWaterMark;

    while (true) {
        ws.cleanupClients();
        notifySensorData();    // Solo sensores — ~180 bytes vs ~940

        // ── Random FX cycling (ESP32-side timer) ──
        // Random VU cycles in TaskLEDControl at the LED cadence (~20 ms).
        bool didSwitchRandomFX = false;
        if (randomMode == 1 && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            if (randomMode == 1 && !randomFXPool.empty() && randomFXDuration > 0) {
                const unsigned long now = millis();
                if (now - lastRandomSwitch >= (unsigned long)randomFXDuration * 1000UL) {
                    int nextId;
                    if (randomFXMode == "playlist") {
                        randomPlaylistIndex %= randomFXPool.size();
                        nextId = randomFXPool[randomPlaylistIndex];
                        randomPlaylistIndex = (randomPlaylistIndex + 1) % randomFXPool.size();
                    } else {
                        nextId = randomFXPool[random(randomFXPool.size())];
                    }
                    stripLed.effectId = nextId;
                    if (stripLed.powerState) stripLed.update();
                    lastRandomSwitch = now;
                    didSwitchRandomFX = true;
                }
            }
            xSemaphoreGive(dataMutex);
        }
        if (didSwitchRandomFX) {
            stateGeneration++;
            notifyClients(false);  // Broadcast new FX ID to all clients
        }

        // Periodic broadcast of client list + action log (cada ~15s)
        // notifyWSClientList() tiene dirty-check interno, solo envía si hubo cambios
        static uint8_t wsListCycle = 0;
        if (++wsListCycle >= 5) {
            wsListCycle = 0;
            notifyWSClientList();
        }

        // Monitor stack every 10 cycles
        static uint8_t cycleCount = 0;
        if (++cycleCount >= WEBSOCKET_STACK_CHECK_CYCLES) {
            stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
            if (stackHighWaterMark < STACK_WARNING_THRESHOLD) {
#ifdef DEBUG_WEBSOCKET
                debuglnW("WebSocket task stack running low!");
                debugD("Stack free: ");
                debugD_NUM(stackHighWaterMark, "%u");
                debuglnD(" bytes");
#endif
            }
            cycleCount = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(WEBSOCKET_UPDATE_INTERVAL));
    }
}

// ============================================================================
// TaskBatteryMonitor — Battery monitoring + power management
// ============================================================================

void TaskBatteryMonitor(void *pvParameters) {
    while (true) {
        batt.battMonitor();
        stateGeneration++;  // Signal new battery data available

        // Protect shared variable access
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            lvlCharge = batt.battLvl;
            xSemaphoreGive(dataMutex);
        } else {
#ifdef DEBUG_BATTERY
            debuglnW("Failed to acquire data mutex in BatteryMonitor");
#endif
        }

        // Power source detection
        bool currentlyOnBattery = (!batt.fullBatt && !batt.chargeState);

        if (currentlyOnBattery != onBatteryPower) {
            onPowerSourceChanged(currentlyOnBattery);
        }

        // Check for critical battery level
        if (onBatteryPower && batt.battLvl < BATTERY_CRITICAL_LEVEL) {
            onCriticalBatteryLevel();
        }

        // WebSocket client tracking
        checkWebSocketClients();

        // Power management state machine
        updatePowerStateMachine();

        vTaskDelay(pdMS_TO_TICKS(BATTERY_CHECK_INTERVAL));
    }
}

// ============================================================================
// TaskLEDControl — LED strip effect update
// ============================================================================

void TaskLEDControl(void *pvParameters) {
    // La tira se apaga SÓLO en la transición a apagado (power off o WiFi
    // suspendido). Antes se llamaba clear() (clear + show) en cada iteración, lo
    // que enviaba un frame negro cada 20 ms — 50 shows/s ocupando el RMT y la CPU
    // para nada mientras la lámpara estaba apagada.
    bool wasActive = false;
    while (true) {
        // Buffer del peek declarado fuera del mutex: el snapshot de leds[] se hace
        // dentro y el envío por red después de soltarlo.
        uint8_t peekBuf[4 + N_PIXELS * 3];
        size_t  peekLen = 0;
        uint32_t peekClientId = 0;

        // ⚠ dataMutex protege leds[], stripLed y el suscriptor Peek del race con
        //   handleWebSocketMessage (que corre en el task del WebSocket).
        bool didSwitchRandomVU = false;
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            // Si el enlace WiFi lleva demasiado tiempo caído (AP apagado o fuera
            // de rango), apagamos la tira. Si solo se fue el cliente WebSocket
            // (móvil en reposo / otra app) el WiFi sigue asociado → el efecto
            // continúa. Lo fija TaskWiFiMonitor.
            const bool active = stripLed.powerState && !wifiStripSuspended;
            const unsigned long now = millis();
            if (!active && randomMode == 2) {
                randomMode = 0;
            }
            if (active && randomMode == 2 && !randomVUPool.empty() && randomVUDuration > 0 &&
                now - lastRandomSwitch >= (unsigned long)randomVUDuration * 1000UL) {
                const int previousId = stripLed.effectId;
                const int nextId = chooseNextRandomVUEffect(previousId);
                lastRandomSwitch = now;
                if (nextId >= 0 && nextId != previousId) {
                    stripLed.effectId = nextId;
                    didSwitchRandomVU = true;
                }
            }
            if (active) {
                stripLed.update();
                buildPeekFrame(peekBuf, sizeof(peekBuf), &peekLen, &peekClientId);  // snapshot
            } else if (wasActive) {
                stripLed.clear();
            }
            wasActive = active;
            xSemaphoreGive(dataMutex);
        }
        if (didSwitchRandomVU) {
            stateGeneration++;
            notifyClients(false);  // Broadcast new VU ID to master and slaves
        }

        // ⚠ Fuera del mutex: el envío unicast no retiene estado compartido.
        //   Frames sin hueco en la cola del cliente se omiten para evitar backlog.
        sendPeekFrame(peekClientId, peekBuf, peekLen);

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// ============================================================================
// TaskWiFiMonitor — WiFi connection monitoring
// ============================================================================

void TaskWiFiMonitor(void *pvParameters) {
    // Detecta transiciones del enlace WiFi para re-registrar mDNS tras un
    // reconnect() y para suspender la tira si el enlace se pierde demasiado
    // tiempo. Arranca ya "conectado" si initWiFi() conectó en setup().
    bool wifiWasConnected = (WiFi.status() == WL_CONNECTED);
    unsigned long wifiLostSince = 0;
    while (true) {
        if (xSemaphoreTake(wifiMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            bool wifiNowConnected = (WiFi.status() == WL_CONNECTED);

            // ── Suspensión por pérdida prolongada del enlace WiFi ──────────────
            //   Se evalúa SIEMPRE (sin depender del modo de energía) para que
            //   funcione aunque powerManagementControllingWiFi esté activo.
            if (wifiNowConnected) {
                wifiStripSuspended = false;
                wifiLostSince = 0;
            } else {
                if (wifiWasConnected) wifiLostEvents++;   // diagnóstico
                if (wifiLostSince == 0) wifiLostSince = millis();
                if (!wifiStripSuspended &&
                    (millis() - wifiLostSince > wifiLostStripTimeoutMs)) {
#ifdef DEBUG_LED
                    debuglnD("📴 WiFi caído demasiado tiempo — apagando tira");
#endif
                    wifiStripSuspended = true;
                }
            }

            // ── AC mode: maintain WiFi ourselves ────────────────────────────
            if (!powerManagementControllingWiFi) {
                if (wifiNowConnected) {
                    if (!wifiWasConnected) {
                        // Re-init mDNS tras un reconnect() propio: sin esto el
                        // responder queda muerto y sml.local no resuelve hasta
                        // un ciclo de energía (bug de Bonjour).
#ifdef DEBUG_NETWORK
                        debuglnD("📡 WiFi reconectado — re-inicializando mDNS");
#endif
                        initMDNS();
                    }
                } else {
                    static unsigned long lastAttempt = 0;
                    if (millis() - lastAttempt > 10000) {   // retry every 10s
#ifdef DEBUG_WIFI
                        debuglnD("WiFi — reconnecting...");
#endif
                        WiFi.reconnect();
                        lastAttempt = millis();
                    }
                }
            }
#ifdef DEBUG_POWER_MANAGEMENT
            // ── Battery mode: power management handles WiFi reconnection ─────
            else {
                static unsigned long lastMsg = 0;
                if (millis() - lastMsg > 10000) {
                    debuglnD("🔋 PM controlling WiFi — TaskWiFiMonitor idle");
                    lastMsg = millis();
                }
            }
#endif
            wifiWasConnected = wifiNowConnected;
            xSemaphoreGive(wifiMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(WIFI_MONITOR_INTERVAL));
    }
}

// ============================================================================
// TaskSensor — Temperature/humidity readings
// ============================================================================

void TaskSensor(void *pvParameters) {
    while (true) {
        readSensor();
        vTaskDelay(pdMS_TO_TICKS(SENSOR_CHECK_INTERVAL));
    }
}

// ============================================================================
// TaskOnboardLED — Built-in LED status indicator
// ============================================================================

void TaskOnboardLED(void *pvParameters) {
    while (true) {
        // LED behavior based on connection status (not power management)
        // Pattern meanings:
        // - WiFi + WebSocket: Mixed short+long blink (200ms+800ms)
        // - WiFi only: Regular 1s blink
        // - Not connected: OFF
        bool wifiConnected = (WiFi.status() == WL_CONNECTED);

        if (wifiConnected && webSocketClientConnected) {
            // WiFi + WebSocket connected: Mixed pattern (short 200ms + long 800ms)
            // Cycle: ON(200ms) -> OFF(300ms) -> ON(500ms) -> OFF(1000ms)
            uint32_t cycleTime = millis() % 2000;
            if (cycleTime < 200) {
                onboard_led.on = true;   // Short blink
            } else if (cycleTime < 500) {
                onboard_led.on = false;  // Short pause
            } else if (cycleTime < 1000) {
                onboard_led.on = true;   // Long blink
            } else {
                onboard_led.on = false;  // Long pause (remainder of 2s cycle)
            }
        } else if (wifiConnected) {
            // WiFi only (no WebSocket): Regular 1s blink
            onboard_led.on = millis() % 1000 < 500;
        } else {
            // Not connected: OFF
            onboard_led.on = false;
        }

        onboard_led.update();
        vTaskDelay(pdMS_TO_TICKS(100)); // Update every 100 ms
    }
}

// ============================================================================
// readSensor — DHT22 temperature/humidity sensor
// ============================================================================

void readSensor() {
    sensors_event_t event;
    int retryCount = 0;
    const int maxRetries = 3;
    const unsigned long retryDelay = 2000;

    while (retryCount < maxRetries) {
        dht.temperature().getEvent(&event);
        if (!isnan(event.temperature)) {
            temp = event.temperature;
            stateGeneration++;  // Signal new temp data
#ifdef DEBUG_TEMPERATURE
            debugD("Temperature: ");
            debugD_FLOAT1(temp);
            debugD("°C\n");
#endif
            break;
        } else {
#ifdef DEBUG_TEMPERATURE
            debuglnD("Error reading temperature! Retrying...");
#endif
            retryCount++;
            vTaskDelay(pdMS_TO_TICKS(retryDelay));
        }
    }

    retryCount = 0;
    while (retryCount < maxRetries) {
        dht.humidity().getEvent(&event);
        if (!isnan(event.relative_humidity)) {
            hum = event.relative_humidity;
            stateGeneration++;  // Signal new humidity data
#ifdef DEBUG_TEMPERATURE
            debugD("Humidity: ");
            debugD_FLOAT1(hum);
            debugD("%\n");
#endif
            break;
        } else {
#ifdef DEBUG_TEMPERATURE
            debuglnD("Error reading humidity! Retrying...");
#endif
            retryCount++;
            vTaskDelay(pdMS_TO_TICKS(retryDelay));
        }
    }
}

// ============================================================================
// initTasks — Create all FreeRTOS tasks
// ============================================================================

void initTasks() {
    xTaskCreatePinnedToCore(TaskWebSocket,       "WebSocketTask",      4096, NULL, 1, &TaskWebSocketHandle,      0);
    xTaskCreatePinnedToCore(TaskBatteryMonitor,  "BatteryMonitorTask", 4096, NULL, 1, &TaskBatteryMonitorHandle, 1);
    xTaskCreatePinnedToCore(TaskLEDControl,      "LEDControlTask",     2048, NULL, 1, &TaskLEDControlHandle,     0);
    xTaskCreatePinnedToCore(TaskWiFiMonitor,     "WiFiMonitorTask",    4096, NULL, 1, &TaskWiFiMonitorHandle,    1);
    xTaskCreatePinnedToCore(TaskSensor,          "SensorTask",         2048, NULL, 1, &TaskSensorHandle,         0);
    xTaskCreatePinnedToCore(TaskOnboardLED,      "LEDOnboardTask",     2048, NULL, 1, &TaskOnboardLEDHandle,     1);
}
