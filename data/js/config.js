/* ──────────────────────────────────────────────────────────────────────────────
   config.js — SML Config Tab — System Info only
   NOTE: WiFi/LED save handlers live in main.js (single source of truth).
   ────────────────────────────────────────────────────────────────────────────── */

// ============================================================================
// SYSTEM INFO UPDATES (called from main.js via WebSocket data)
// ============================================================================

function updateSystemInfo(data) {
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
      el.textContent = str;
    }
  }

  if (data.heap !== undefined) {
    const el = document.getElementById('sysHeap');
    if (el) el.textContent = data.heap + ' KB';
  }

  if (data.rssi !== undefined) {
    const el = document.getElementById('sysRSSI');
    if (el) el.textContent = data.rssi + ' dBm';
  }

  if (data.ip !== undefined) {
    const el = document.getElementById('deviceIP');
    if (el) el.textContent = data.ip;
    const ipEl = document.getElementById('sysIP');
    if (ipEl) ipEl.textContent = data.ip;
  }

  if (data.n !== undefined) {
    const el = document.getElementById('sysLEDs');
    if (el) el.textContent = data.n;
    const countEl = document.getElementById('ledCount');
    if (countEl && !countEl.dataset.userChanged) countEl.value = data.n;
  }

  if (data.ver !== undefined) {
    const el = document.getElementById('sysVersion');
    if (el) el.textContent = data.ver;
  }
}
