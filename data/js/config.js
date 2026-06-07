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
