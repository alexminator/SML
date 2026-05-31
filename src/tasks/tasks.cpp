// ──────────────────────────────────────────────────────────────────────────────
// tasks.cpp — FreeRTOS Task implementations
// ──────────────────────────────────────────────────────────────────────────────
#include "tasks.h"
#include "../state/AppState.h"
#include "net/WebSocket.h"
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
        notifyClients();

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
    while (true) {
        if (stripLed.powerState) {
            stripLed.update();
        } else {
            stripLed.clear();
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// ============================================================================
// TaskWiFiMonitor — WiFi connection monitoring
// ============================================================================

void TaskWiFiMonitor(void *pvParameters) {
    while (true) {
        if (xSemaphoreTake(wifiMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // ── AC mode: maintain WiFi ourselves ────────────────────────────
            if (!powerManagementControllingWiFi) {
                if (WiFi.status() != WL_CONNECTED) {
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
            // ── Battery mode: power management handles it ───────────────────
            else {
                static unsigned long lastMsg = 0;
                if (millis() - lastMsg > 10000) {
                    debuglnD("🔋 PM controlling WiFi — TaskWiFiMonitor idle");
                    lastMsg = millis();
                }
            }
#endif
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
    xTaskCreatePinnedToCore(TaskBatteryMonitor,  "BatteryMonitorTask", 2048, NULL, 1, &TaskBatteryMonitorHandle, 1);
    xTaskCreatePinnedToCore(TaskLEDControl,      "LEDControlTask",     2048, NULL, 1, &TaskLEDControlHandle,     0);
    xTaskCreatePinnedToCore(TaskWiFiMonitor,     "WiFiMonitorTask",    2048, NULL, 1, &TaskWiFiMonitorHandle,    1);
    xTaskCreatePinnedToCore(TaskSensor,          "SensorTask",         2048, NULL, 1, &TaskSensorHandle,         0);
    xTaskCreatePinnedToCore(TaskOnboardLED,      "LEDOnboardTask",     2048, NULL, 1, &TaskOnboardLEDHandle,     1);
}
