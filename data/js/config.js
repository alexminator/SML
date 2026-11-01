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
    // Sync browser time for action log timestamps
    syncWsTime(data.uptime);
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
    setDataValue(document.getElementById('sysIP'), data.ip);
  }

  // ── VERSION ──
  if (data.ver !== undefined) {
    setDataValue(document.getElementById('sysVersion'), data.ver);
  }

  // ── MAC ADDRESS ──
  if (data.mac !== undefined) {
    setDataValue(document.getElementById('sysMAC'), data.mac);
  }

  // ── SSID placeholder (cuando el input está vacío) ──
  if (data.ssid !== undefined) {
    const ssidEl = document.getElementById('wifiSsid');
    if (ssidEl && !ssidEl.value) ssidEl.placeholder = data.ssid;
  }

  // ── WiFi-loss strip timeout (segundos) ──
  if (data.wifiTimeout !== undefined) {
    if (typeof SML !== 'undefined') SML.wifiTimeout = data.wifiTimeout;
    const t = document.getElementById('wifiTimeoutInput');
    if (t && document.activeElement !== t) t.value = data.wifiTimeout;
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

  // Keep client list reference for re-renders when only log arrives
  if (clients) container._clients = clients;

  // Merge action log entries into the local accumulated history (avoid duplicates).
  // This prevents the master's old log from disappearing when a slave connects
  // and the ESP32's circular buffer (WS_LOG_SIZE=64) overwrites the oldest master
  // entries with the slave's newer ones.
  if (actionLog && Array.isArray(actionLog)) {
    if (!container._actionLog) container._actionLog = [];
    const existingKeys = new Set(container._actionLog.map(e => _actionLogKey(e)));
    for (const entry of actionLog) {
      const key = _actionLogKey(entry);
      if (!existingKeys.has(key)) {
        container._actionLog.push(entry);
        existingKeys.add(key);
      }
    }
  }

  // Use stored clients if none provided (standalone actionLog message)
  clients = container._clients || [];

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
    const log = (container._actionLog || []).filter(e => e.c === c.id).slice(-10);
    if (log.length > 0) {
      const logContainer = document.createElement('div');
      logContainer.className = 'ws-client-actions';
      log.forEach(e => {
        const entry = document.createElement('div');
        entry.className = 'ws-client-action';
        entry.innerHTML = _formatActionEntry(e);
        // Texto completo como tooltip (sin la hora): si la entrada se corta, al
        // pasar el ratón se lee entera sin ocupar dos filas.
        const textEl = entry.querySelector('.ws-action-text');
        entry.title = (textEl ? textEl.textContent : entry.textContent).replace(/\s+/g, ' ').trim();
        logContainer.appendChild(entry);
      });
      wrapper.appendChild(logContainer);
    }

    container.appendChild(wrapper);
  });
}

/**
 * Build a unique key for an action log entry to avoid duplicates when
 * merging the ESP32's circular-buffer log into the browser's local history.
 * @param {Object} e - Log entry {t, c, ty, v1, v2, v3}
 * @returns {string} unique key
 */
function _actionLogKey(e) {
  return `${e.t}|${e.c}|${e.ty}|${e.v1}|${e.v2}|${e.v3}`;
}

/**
 * Format a single action log entry into HTML.
 * @param {Object} e - Log entry {t, ty, v1, v2, v3}
 * @returns {string} HTML string
 */
function _formatActionEntry(e) {
  const timeStr = _formatWsTime(e.t);
  // Cada entrada: hora | icono | texto. El texto va en un span propio con
  // ellipsis para que una accion larga (params, efecto) no rompa en dos filas.
  const row = (icon, text, color) =>
    `<span class="ws-action-time">${timeStr}</span>` +
    `<span class="ws-action-icon ${icon}"${color ? ` style="color:${color}"` : ''}></span>` +
    `<span class="ws-action-text">${text}</span>`;

  switch (e.ty) {
    case 0: // effect change
      return row('fas fa-star', `Effect → <strong>${effectIdToName[e.v1] || 'ID ' + e.v1}</strong>`, 'var(--accent)');
    case 1: // color
      return row('fas fa-palette', `<span class="ws-color-swatch" style="background:rgb(${e.v1},${e.v2},${e.v3})"></span> RGB ${e.v1},${e.v2},${e.v3}`, 'var(--accent)');
    case 2: // brightness
      return row('fas fa-sun', `Bright ${e.v1}`, 'var(--accent-warning)');
    case 3: // params
      return row('fas fa-sliders-h', `Params · speed ${e.v2} · int ${e.v3}`, 'var(--accent-secondary)');
    case 4: // power toggle
      return row('fas fa-power-off', e.v1 ? 'ON' : 'OFF', e.v1 ? 'var(--accent-success)' : 'var(--accent-danger)');
    case 5: // lamp
      return row('fas fa-lightbulb', `Lamp ${e.v1 ? 'ON' : 'OFF'}`, e.v1 ? 'var(--accent-warning)' : 'var(--text-secondary)');
    case 6: // bt
      return row('fab fa-bluetooth-b', `BT ${e.v1 ? 'ON' : 'OFF'}`, 'var(--accent)');
    case 7: // random (v1=1 randomFX, v1=2 randomVU, v1=3 randomConfig)
      return row('fas fa-shuffle', `Random ${e.v1 === 2 ? 'VU' : 'FX'} ON`, 'var(--accent)');
    case 8: // batería (visualización de carga)
      return row('fas fa-battery-full', `Battery ${e.v1 ? 'ON' : 'OFF'}`, e.v1 ? '#5cb85c' : 'var(--text-secondary)');
    case 9: // temperatura (visualización de sensor)
      return row('fas fa-thermometer-half', `Temp ${e.v1 ? 'ON' : 'OFF'}`, e.v1 ? '#ff7f0e' : 'var(--text-secondary)');
    case 10: // volume up
      return row('fas fa-volume-up', 'Volume +', 'var(--accent)');
    case 11: // volume down
      return row('fas fa-volume-down', 'Volume −', 'var(--accent)');
    case 12: // skip left (previous track)
      return row('fas fa-step-backward', 'Previous', 'var(--accent)');
    case 13: // skip right (next track)
      return row('fas fa-step-forward', 'Next', 'var(--accent)');
    case 14: // play/pause
      return row('fas fa-play', 'Play / Pause', 'var(--accent)');
    default:
      return row('fas fa-circle-info', `Action #${e.ty}`, 'var(--text-secondary)');
  }
}

// ============================================================================
// TIME SYNC — Convierte uptime del ESP32 a hora local del navegador
// ============================================================================
let _wsTimeRef = null;  // { browserMs, espUptime }

/** Llamar cuando se recibe data.uptime desde el ESP32 */
function syncWsTime(espUptime) {
  _wsTimeRef = { browserMs: Date.now(), espUptime };
}

/** Formatea un timestamp del ESP32 como hora local del navegador */
function _formatWsTime(espUptime) {
  // Hora corta: h:mm AM/PM, sin segundos (en el log no aportan nada y ensucian
  // la columna). Si la entrada no es de hoy, se antepone la fecha corta.
  const timeOpts = { hour: 'numeric', minute: '2-digit', hour12: true };
  if (!_wsTimeRef) {
    // Sin sincronía: uptime en formato HH:MM (no hay AM/PM que aplicar).
    const h = String(Math.floor(espUptime / 3600)).padStart(2, '0');
    const m = String(Math.floor((espUptime % 3600) / 60)).padStart(2, '0');
    return `${h}:${m}`;
  }
  const msAgo = (_wsTimeRef.espUptime - espUptime) * 1000;
  const t = new Date(_wsTimeRef.browserMs - msAgo);
  const now = new Date();
  const hm = t.toLocaleTimeString('en-US', timeOpts);
  if (t.toDateString() === now.toDateString()) return hm;
  const day = t.toLocaleDateString('en-US', { month: 'short', day: 'numeric' });
  return `${day}, ${hm}`;
}

/**
 * Convert ESP32 uptime (seconds) to browser Date timestamp (ms).
 * Uses the same sync point as _formatWsTime for consistent time axes.
 * @param {number} espUptime — uptime in seconds from ESP32
 * @returns {number} browser Date.now() equivalent
 */
function _battLogTimeToBrowser(espUptime) {
  if (!_wsTimeRef) {
    // Fallback: treat as relative time from now
    return Date.now() - espUptime * 1000;
  }
  const msAgo = (_wsTimeRef.espUptime - espUptime) * 1000;
  return _wsTimeRef.browserMs - msAgo;
}

/**
 * Debug helper — llama desde la consola del navegador (F12 → Console):
 *   dumpActionLog()         → tabla con todas las entradas recibidas
 *   dumpActionLog('raw')    → JSON completo para copiar/pegar
 */
window.dumpActionLog = function(mode) {
  const container = document.getElementById('wsClientList');
  if (!container || !container._actionLog) {
    console.log('📭 No hay action log en el navegador. Abre la pestaña Config y espera datos.');
    return;
  }
  const log = container._actionLog;
  if (mode === 'raw') {
    console.log(JSON.stringify(log, null, 2));
  } else {
    console.table(log.map(e => ({
      t: e.t, c: e.c, ty: e.ty, v1: e.v1, v2: e.v2, v3: e.v3
    })));
  }
  console.log(`📊 Total entries in buffer: ${log.length}`);
};

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
