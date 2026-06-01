/* ──────────────────────────────────────────────────────────────────────────────
   config.js — SML Config Tab
   Theme selector, WiFi settings, LED strip config, system info
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

// ============================================================================
// WIFI CREDENTIALS
// ============================================================================

function saveWiFiConfig() {
  const ssid = document.getElementById('wifiSsid')?.value.trim();
  const pass = document.getElementById('wifiPass')?.value;

  if (!ssid) {
    showToast('Please enter an SSID');
    return;
  }

  if (typeof sendCmd === 'function') {
    sendCmd({ ssid: ssid, pass: pass || '' });
  }

  showToast('WiFi saved. Reconnecting...');
}

// ============================================================================
// LED CONFIG
// ============================================================================

function saveLEDStripConfig() {
  const countEl = document.getElementById('ledCount');
  const maEl = document.getElementById('ledMaxMA');

  const count = parseInt(countEl?.value);
  const maxMA = parseInt(maEl?.value) || 500;

  if (!count || count < 1 || count > 300) {
    showToast('LED count must be 1-300');
    return;
  }

  if (typeof sendCmd === 'function') {
    sendCmd({ ledcfg: { n: count, ma: maxMA } });
  }

  showToast('LED config saved. Rebooting...');
}

// ============================================================================
// THEME SELECTOR (handled in main.js, but we bind the UI here)
// ============================================================================

document.addEventListener('DOMContentLoaded', () => {
  // WiFi save button
  const wifiBtn = document.getElementById('wifiSaveBtn');
  if (wifiBtn) wifiBtn.addEventListener('click', saveWiFiConfig);

  // LED save button
  const ledBtn = document.getElementById('ledSaveBtn');
  if (ledBtn) ledBtn.addEventListener('click', saveLEDStripConfig);

  // Mark LED count as user-changed
  const ledCount = document.getElementById('ledCount');
  if (ledCount) {
    ledCount.addEventListener('input', () => {
      ledCount.dataset.userChanged = 'true';
    });
  }

  // Enter key on WiFi password field
  const wifiPass = document.getElementById('wifiPass');
  if (wifiPass) {
    wifiPass.addEventListener('keydown', (e) => {
      if (e.key === 'Enter') saveWiFiConfig();
    });
  }
});
