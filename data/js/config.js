/* ──────────────────────────────────────────────────────────────────────────────
   config.js — SML Config Tab — System Info & WiFi management
   Called from main.js handleMessage() for config-related fields.
   Uses global helpers: setDataValue(), setDataHTML(), showToast()
   ────────────────────────────────────────────────────────────────────────────── */

/**
 * Update Config tab UI elements from WebSocket data.
 * Called by main.js handleMessage() — single source of truth for config fields.
 * @param {Object} data - WebSocket JSON payload from ESP32
 */
function updateSystemInfo(data) {
  // ── UPTIME ──
  if (data.uptime !== undefined) {
    const el = document.getElementById('sysUptime');
    if (el) {
      const s = data.uptime;
      const d = Math.floor(s / 86400);
      const h = Math.floor((s % 86400) / 3600);
      const m = Math.floor((s % 3600) / 60);
      let str = '';
      if (d > 0) str += d + 'd ';
      if (h > 0 || d > 0) str += h + 'h ';
      str += m + 'm';
      setDataValue(el, str);
    }
  }

  // ── FREE HEAP ──
  if (data.heap !== undefined) {
    setDataValue(document.getElementById('sysHeap'), data.heap, ' KB');
  }

  // ── RSSI / SIGNAL ──
  if (data.rssi !== undefined) {
    setDataValue(document.getElementById('sysRSSI'), data.rssi, ' dBm');
  }

  // ── IP ADDRESS ──
  if (data.ip !== undefined) {
    setDataValue(document.getElementById('deviceIP'), data.ip);
    setDataValue(document.getElementById('sysIP'), data.ip);
  }

  // ── VERSION ──
  if (data.ver !== undefined) {
    setDataValue(document.getElementById('sysVersion'), data.ver);
  }

  // ── SSID placeholder (cuando el input está vacío) ──
  if (data.ssid !== undefined) {
    const ssidEl = document.getElementById('wifiSsid');
    if (ssidEl && !ssidEl.value) ssidEl.placeholder = data.ssid;
  }

  // ── LED COUNT (static — defined in config.h) ──
  const ledsEl = document.getElementById('sysLEDs');
  if (ledsEl && ledsEl.textContent === '--') {
    setDataValue(ledsEl, 24);
  }
}

/**
 * Update WebSocket client list in Config tab.
 * Called by main.js handleMessage() when wsClientList is received.
 * @param {Array} clients - Array of {id, ip, master} objects
 * @param {Array} [actionLog] - Optional array of {t, c, ty, v1, v2, v3} log entries
 */
function updateWSClientList(clients, actionLog) {
  const container = document.getElementById('wsClientList');
  if (!container) return;

  // Keep log reference for re-renders when only log arrives
  if (actionLog) container._actionLog = actionLog;

  // Clear previous content
  container.innerHTML = '';

  if (!clients || clients.length === 0) {
    container.innerHTML = '<div class="ws-client-empty">No clients connected</div>';
    return;
  }

  clients.forEach(c => {
    const wrapper = document.createElement('div');
    wrapper.className = 'ws-client-wrapper';

    // ── Client header ──
    const item = document.createElement('div');
    item.className = 'ws-client-item';

    const role = c.master ? 'master' : 'slave';
    const roleLabel = c.master ? 'Master' : 'Slave';
    const roleIcon = c.master
      ? '<span class="fas fa-crown"></span>'
      : '<span class="fas fa-link"></span>';

    item.innerHTML = `
      <span class="ws-client-badge ${role}">${roleIcon} ${roleLabel}</span>
      <span class="ws-client-ip">${c.ip}</span>
      <span class="ws-client-id">#${c.id}</span>
    `;
    wrapper.appendChild(item);

    // ── Action log for this client ──
    const log = (container._actionLog || []).filter(e => e.c === c.id).slice(-4);
    if (log.length > 0) {
      const logContainer = document.createElement('div');
      logContainer.className = 'ws-client-actions';
      log.forEach(e => {
        const entry = document.createElement('div');
        entry.className = 'ws-client-action';
        entry.innerHTML = _formatActionEntry(e);
        logContainer.appendChild(entry);
      });
      wrapper.appendChild(logContainer);
    }

    container.appendChild(wrapper);
  });
}

/**
 * Format a single action log entry into HTML.
 * @param {Object} e - Log entry {t, ty, v1, v2, v3}
 * @returns {string} HTML string
 */
function _formatActionEntry(e) {
  const uptime = _fmtUptime(e.t);
  switch (e.ty) {
    case 0: // effect change
      return `<span class="ws-action-time">${uptime}</span> <span class="fas fa-star" style="color:var(--accent)"></span> Effect → <strong>${effectIdToName[e.v1] || 'ID ' + e.v1}</strong>`;
    case 1: // color
      return `<span class="ws-action-time">${uptime}</span> <span class="fas fa-palette" style="color:var(--accent)"></span> Color <span class="ws-color-swatch" style="background:rgb(${e.v1},${e.v2},${e.v3})"></span> RGB(${e.v1}, ${e.v2}, ${e.v3})`;
    case 2: // brightness
      return `<span class="ws-action-time">${uptime}</span> <span class="fas fa-sun" style="color:var(--accent-warning)"></span> Brightness → ${e.v1}`;
    case 3: // params
      return `<span class="ws-action-time">${uptime}</span> <span class="fas fa-sliders-h" style="color:var(--accent-secondary)"></span> Params (speed=${e.v2}, intensity=${e.v3})`;
    case 4: // power toggle
      return `<span class="ws-action-time">${uptime}</span> <span class="fas fa-power-off" style="color:${e.v1 ? 'var(--accent-success)' : 'var(--accent-danger)'}"></span> ${e.v1 ? 'ON' : 'OFF'}`;
    case 5: // lamp
      return `<span class="ws-action-time">${uptime}</span> <span class="fas fa-lightbulb" style="color:${e.v1 ? 'var(--accent-warning)' : 'var(--text-secondary)'}"></span> Lamp ${e.v1 ? 'ON' : 'OFF'}`;
    case 6: // bt
      return `<span class="ws-action-time">${uptime}</span> <span class="fab fa-bluetooth-b" style="color:${e.v1 ? '#0072ff' : 'var(--text-secondary)'}"></span> BT ${e.v1 ? 'ON' : 'OFF'}`;
    case 7: // random
      return `<span class="ws-action-time">${uptime}</span> <span class="fas fa-shuffle" style="color:var(--accent)"></span> Random ${e.v1 === 1 ? 'FX' : 'VU'} ${e.v1 ? 'ON' : 'OFF'}`;
    default:
      return `<span class="ws-action-time">${uptime}</span> Action #${e.ty}`;
  }
}

/** Format uptime seconds → "HH:MM:SS" */
function _fmtUptime(sec) {
  const h = String(Math.floor(sec / 3600)).padStart(2, '0');
  const m = String(Math.floor((sec % 3600) / 60)).padStart(2, '0');
  const s = String(sec % 60).padStart(2, '0');
  return `${h}:${m}:${s}`;
}

// ============================================================================
// POLLING — Config tab auto-refresh
// ============================================================================
// Polls the server every 10s for client list + action log when config tab
// is active. Falls back gracefully if sendCmd/switchTab are not available.
// ============================================================================

(function() {
  const POLL_INTERVAL = 10000; // 10 seconds

  // Store the current tab for cross-module checking
  let currentTab = '';

  // Listen for tab switches
  document.addEventListener('tabSwitch', (e) => {
    currentTab = e.detail?.tab || '';
  });

  // Also hook into window.SML if available
  function isConfigTabActive() {
    if (typeof SML !== 'undefined' && SML.currentTab) {
      return SML.currentTab === 'tabConfig';
    }
    return currentTab === 'tabConfig';
  }

  setInterval(() => {
    if (!isConfigTabActive()) return;
    if (typeof sendCmd !== 'function') return;
    sendCmd({ action: 'refreshClientList' });
  }, POLL_INTERVAL);
})();
