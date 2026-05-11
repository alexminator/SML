/*=============== SML Main JavaScript - WebSocket & Data Routing ===============*/
/* SML Web Interface v2.1 - Main Controller */

// WebSocket connection
let websocket = null;
let connected = false;
let reconnectAttempts = 0;
const MAX_RECONNECT_ATTEMPTS = 10;
const reconnectIntervals = [1000, 2000, 4000, 8000, 15000]; // Exponential backoff

// Application state
const appState = {
  lampOn: false,
  neoOn: false,
  btOn: false,
  brightness: 130,
  color: { r: 255, g: 255, b: 255 },
  currentEffect: 'None'
};

// ============================================================================
// WebSocket Connection Management
// ============================================================================

function connectWebSocket() {
  const wsProtocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
  const wsUrl = `${wsProtocol}//${window.location.hostname}/ws`;

  console.log(`[WebSocket] Connecting to ${wsUrl}`);

  try {
    websocket = new WebSocket(wsUrl);

    websocket.onopen = handleWebSocketOpen;
    websocket.onclose = handleWebSocketClose;
    websocket.onerror = handleWebSocketError;
    websocket.onmessage = handleWebSocketMessage;
  } catch (error) {
    console.error('[WebSocket] Connection failed:', error);
    scheduleReconnect();
  }
}

function handleWebSocketOpen(event) {
  console.log('[WebSocket] Connected');
  connected = true;
  reconnectAttempts = 0;

  // Update UI to show connected state
  updateConnectionStatus(true);

  // Send initial status request if needed
  // (ESP32 sends updates automatically via notifyClients)
}

function handleWebSocketClose(event) {
  console.log('[WebSocket] Disconnected');
  connected = false;
  updateConnectionStatus(false);

  // Attempt reconnection
  scheduleReconnect();
}

function handleWebSocketError(error) {
  console.error('[WebSocket] Error:', error);
  updateConnectionStatus(false);
}

function handleWebSocketMessage(event) {
  try {
    // Parse JSON message
    const data = JSON.parse(event.data);

    // Route message based on content
    if (data.leds && Array.isArray(data.leds)) {
      // LED preview data for Peek tab
      handleLEDData(data);
    } else {
      // General status update
      handleStatusUpdate(data);
    }
  } catch (error) {
    console.error('[WebSocket] Failed to parse message:', error);
  }
}

function scheduleReconnect() {
  if (reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
    console.error('[WebSocket] Max reconnection attempts reached');
    updateConnectionStatus(false, 'max_attempts');
    return;
  }

  // Calculate backoff interval
  const intervalIndex = Math.min(reconnectAttempts, reconnectIntervals.length - 1);
  const interval = reconnectIntervals[intervalIndex];

  reconnectAttempts++;

  console.log(`[WebSocket] Reconnecting in ${interval}ms (attempt ${reconnectAttempts}/${MAX_RECONNECT_ATTEMPTS})`);
  updateConnectionStatus(false, 'reconnecting');

  setTimeout(connectWebSocket, interval);
}

function updateConnectionStatus(isConnected, status = 'connected') {
  const wifiSignal = document.getElementById('wifiSignal');

  if (wifiSignal) {
    if (isConnected) {
      // Will be updated by status messages
      wifiSignal.classList.add('sml-wifi-signal');
    } else {
      if (status === 'reconnecting') {
        wifiSignal.textContent = 'Reconnecting...';
        wifiSignal.className = 'sml-wifi-signal waveStrength-1';
      } else if (status === 'max_attempts') {
        wifiSignal.textContent = 'Disconnected';
        wifiSignal.className = 'no-signal';
      } else {
        wifiSignal.textContent = 'Disconnected';
        wifiSignal.className = 'no-signal';
      }
    }
  }
}

// ============================================================================
// LED Data Handler (Peek Tab)
// ============================================================================

function handleLEDData(data) {
  // Validate data structure
  if (!data.leds || !Array.isArray(data.leds)) {
    console.warn('[LED Data] Invalid LED array');
    return;
  }

  if (!window.ledPreview) {
    // LEDPreview not initialized yet
    console.warn('[LED Data] LEDPreview not available');
    return;
  }

  // Convert LED data format if needed
  // ESP32 sends: [{"r":0,"g":0,"b":0}, ...]
  // LEDPreview expects: array of {r,g,b} objects (already correct)

  try {
    // Extract metadata
    const ledArray = data.leds;
    const realCount = data.realCount || N_PIXELS || 24;
    const previewCount = data.previewCount || ledArray.length;
    const effectName = data.effect || 'Unknown';

    // Update LED preview
    window.ledPreview.update(ledArray, effectName);

    // Update LED count in preview
    window.ledPreview.updateLEDCount(realCount);

    // Debug logging
    if (window.DEBUG_MODE) {
      console.log(`[LED Data] ${previewCount} LEDs, effect: ${effectName}`);
    }
  } catch (error) {
    console.error('[LED Data] Failed to update preview:', error);
  }
}

// ============================================================================
// Status Update Handler
// ============================================================================

function handleStatusUpdate(data) {
  // Update WiFi signal
  if (data.bars) {
    const wifiSignal = document.getElementById('wifiSignal');
    if (wifiSignal) {
      wifiSignal.textContent = data.bars.replace('waveStrength-', 'Bars: ');
      wifiSignal.className = `sml-wifi-signal ${data.bars}`;
    }
  }

  // Update battery
  if (data.level !== undefined) {
    updateBatteryDisplay(data.level, data.charging, data.fullbatt);
  }

  // Update temperature and humidity
  if (data.temperature !== undefined) {
    updateTemperatureDisplay(data.temperature);
  }

  if (data.humidity !== undefined) {
    updateHumidityDisplay(data.humidity);
  }

  // Update lamp/neo/bt status
  if (data.lampstatus !== undefined) {
    appState.lampOn = (data.lampstatus === 'on');
    updateToggleState('lamp', appState.lampOn);
  }

  if (data.neostatus !== undefined) {
    appState.neoOn = (data.neostatus === 'on');
    updateToggleState('Neo', appState.neoOn);
  }

  if (data.btstatus !== undefined) {
    appState.btOn = (data.btstatus === 'on');
    updateToggleState('Bluetooth', appState.btOn);
  }

  // Update brightness
  if (data.neobrightness !== undefined) {
    appState.brightness = data.neobrightness;
  }

  // Update color
  if (data.color && typeof data.color === 'object') {
    appState.color = {
      r: data.color.r || 255,
      g: data.color.g || 255,
      b: data.color.b || 255
    };
  }

  // Update WiFi info in Config tab
  if (data.ssid) {
    const ssidElement = document.getElementById('currentSSID');
    if (ssidElement) ssidElement.textContent = data.ssid;
  }

  if (data.ip) {
    const ipElement = document.getElementById('currentIP');
    if (ipElement) ipElement.textContent = data.ip;
  }

  if (data.rssi !== undefined) {
    const signalElement = document.getElementById('currentSignal');
    if (signalElement) signalElement.textContent = `${data.rssi} dBm`;
  }
}

// ============================================================================
// UI Update Functions
// ============================================================================

function updateBatteryDisplay(level, charging, full) {
  const batteryPercent = document.getElementById('batteryPercent');
  const batteryIcon = document.getElementById('batteryIcon');
  const batteryLevel = document.getElementById('battlevel');
  const batteryCharge = document.getElementById('battCharge');
  const battVolt = document.getElementById('battVolt');

  if (batteryPercent) {
    batteryPercent.textContent = `${level}%`;
  }

  if (batteryIcon) {
    // Choose icon based on level and charging state
    if (full) {
      batteryIcon.className = 'fas fa-battery-full';
    } else if (charging) {
      batteryIcon.className = 'fas fa-battery-quarter fa-bolt';
    } else {
      const level10 = Math.floor(level / 10);
      const iconLevel = level10 === 10 ? 'full' : `-${level10 * 10}`;
      batteryIcon.className = `fas fa-battery${iconLevel}`;
    }
  }

  if (batteryLevel) {
    batteryLevel.textContent = level;
  }

  if (batteryCharge) {
    if (full) {
      batteryCharge.textContent = 'Fully Charged';
      batteryCharge.innerHTML = '<span class="fas fa-check"></span> Full';
    } else if (charging) {
      batteryCharge.textContent = 'Charging';
      batteryCharge.innerHTML = '<span class="fas fa-plug"></span> Charging';
    } else if (level < 20) {
      batteryCharge.textContent = 'Low Battery';
      batteryCharge.innerHTML = '<span class="fas fa-exclamation-triangle"></span> Low';
    } else {
      batteryCharge.textContent = 'On Battery';
      batteryCharge.innerHTML = '<span class="fas fa-battery-three-quarters"></span> Battery';
    }
  }

  if (battVolt && data.battVoltage) {
    battVolt.textContent = `${data.battVoltage.toFixed(2)}V`;
  }
}

function updateTemperatureDisplay(temp) {
  // Weather tab temperature
  const tempElement = document.getElementById('temperatura');
  if (tempElement) {
    tempElement.textContent = `${temp.toFixed(1)}°C`;
  }
}

function updateHumidityDisplay(humidity) {
  const humidityElement = document.getElementById('humedad');
  if (humidityElement) {
    humidityElement.textContent = `${humidity.toFixed(1)}%`;
  }
}

function updateToggleState(id, state) {
  const toggle = document.getElementById(id);
  if (toggle && toggle.classList.contains('on') !== state) {
    toggle.classList.toggle('on', state);
  }
}

// ============================================================================
// WebSocket Action Senders
// ============================================================================

function sendWebSocketAction(action, params = {}) {
  if (!connected || !websocket) {
    console.warn('[WebSocket] Not connected, cannot send action');
    return;
  }

  const message = {
    action: action,
    ...params
  };

  try {
    websocket.send(JSON.stringify(message));
    console.log('[WebSocket] Sent:', action, params);
  } catch (error) {
    console.error('[WebSocket] Failed to send message:', error);
  }
}

// ============================================================================
// Initialization
// ============================================================================

document.addEventListener('DOMContentLoaded', () => {
  console.log('SML Web Interface v2.1 - Initializing...');

  // Initialize WebSocket connection
  connectWebSocket();

  // Update status bar placeholders
  const wifiSignal = document.getElementById('wifiSignal');
  const batteryPercent = document.getElementById('batteryPercent');

  if (wifiSignal) {
    wifiSignal.textContent = 'Connecting...';
  }

  if (batteryPercent) {
    batteryPercent.textContent = '--%';
  }

  // Make sendWebSocketAction available globally for other scripts
  window.sendWebSocketAction = sendWebSocketAction;
  window.SMLState = appState;

  console.log('SML Web Interface v2.1 - Ready');
});

// ============================================================================
// Cleanup on Page Unload
// ============================================================================

window.addEventListener('beforeunload', () => {
  if (websocket) {
    websocket.close();
  }
});
