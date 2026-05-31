// ──────────────────────────────────────────────────────────────────────────────
// tasks.h — FreeRTOS Task declarations
// ──────────────────────────────────────────────────────────────────────────────
#pragma once

#include <Arduino.h>

// ============================================================================
// Task Handles
// ============================================================================

extern TaskHandle_t TaskWebSocketHandle;
extern TaskHandle_t TaskBatteryMonitorHandle;
extern TaskHandle_t TaskLEDControlHandle;
extern TaskHandle_t TaskWiFiMonitorHandle;
extern TaskHandle_t TaskSensorHandle;
extern TaskHandle_t TaskOnboardLEDHandle;

// ============================================================================
// Task Functions
// ============================================================================

void TaskWebSocket(void *pvParameters);
void TaskBatteryMonitor(void *pvParameters);
void TaskLEDControl(void *pvParameters);
void TaskWiFiMonitor(void *pvParameters);
void TaskSensor(void *pvParameters);
void TaskOnboardLED(void *pvParameters);

// ============================================================================
// Helpers
// ============================================================================

void readSensor();
void initTasks();
