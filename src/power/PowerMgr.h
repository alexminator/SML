// ──────────────────────────────────────────────────────────────────────────────
// PowerMgr.h — Power Management State Machine
// ──────────────────────────────────────────────────────────────────────────────
#pragma once

#include "state/AppState.h"

// State machine transitions
void transitionToState(PowerState newState);
void applyStateConfiguration(PowerState state);

// Event handlers
void onPowerSourceChanged(bool nowOnBattery);
void onCriticalBatteryLevel();

// State handlers
void handleBatteryConnectingState();
void handleBatterySleepState();

// Main update
void updatePowerStateMachine();

// Periodic checks
void checkWebSocketClients();
