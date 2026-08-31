/* ──────────────────────────────────────────────────────────────────────────────
   main.js — SML Web UI orchestrator
   Globals, app state, tabs, theme, status bar, DOMContentLoaded init
   ──────────────────────────────────────────────────────────────────────────────
   ⚠ CRITICAL: No snprintf() anywhere — ESP32 crashes with format specifiers.
   All values sent as raw JSON numbers, never pre-formatted strings.

   Protocol (ESP32 → Client):
     {battVoltage, level, charging, fullbatt,
      temperature, humidity, lampstatus, neostatus, btstatus,
      neobrightness, ssid, ip, rssi,
      color: {r,g,b}, [effectName]: "on"/"off", params: {...}}

   Protocol (Client → ESP32):
     {"action": "toggle"} | "lamp" | "music" | "play-pause" | ...
     {"action": "slider", "brightness": 130}
     {"action": "picker", "color": {r,g,b}}
     {"effectId": 5}
     {"action": "setParams", "effectId": 5, "speed": 120, ...}
   ────────────────────────────────────────────────────────────────────────────── */

// ============================================================================
// GLOBAL VARIABLES (required by player.js, battery.js)
// ============================================================================

let websocket = null;
let json = { action: '' };   // Reused for sending commands (player.js pattern)
const batt = { level: 0, charging: false, fullbatt: false };

// Effect ID → human-readable name (used by config.js for action log)
const effectIdToName = {
  0: 'Solid',
  1: 'Fire', 2: 'Moving Dot', 3: 'Rainbow Beat', 4: 'RWB', 5: 'Ripple',
  6: 'Balls', 7: 'Juggle', 8: 'Sinelon', 9: 'Comet', 10: 'Breath',
  11: 'Color Sweep', 12: 'Rainbow VU', 13: 'Oldskool VU', 14: 'Rainbow Hue VU',
  15: 'Ripple VU', 16: 'Three Bars VU', 17: 'Ocean VU',
  18: 'Temperature', 19: 'Battery',
  20: 'Color Wipe', 21: 'Theater Chase', 22: 'Running Lights',
  23: 'Dissolve', 24: 'Dual Scan', 25: 'Fade', 26: 'Meteor',
  27: 'Sparkle', 28: 'BPM', 29: 'Plasma', 30: 'Fireworks',
  31: 'Lightning', 32: 'Pride 2015', 33: 'Color Waves', 34: 'Pacifica',
  35: 'TwinkleFOX', 36: 'Aurora', 37: 'Popcorn',
  38: 'Larson Scanner', 39: 'Heartbeat', 40: 'ICU',
  41: 'Sunrise', 42: 'Drip', 43: 'Candle', 44: 'Chunchun',
  45: 'Halloween Eyes',
  47: 'Noisemeter VU', 48: 'DJ Light VU',
};

// ============================================================================
// APP STATE
// ============================================================================

const SML = {
  ws: null,
  wsReconnectTimer: null,
  connected: false,

  // Device state
  powerOn: false,
  brightness: 130,
  r: 255, g: 255, b: 255,
  effectId: 0,
  temp: 0,
  hum: 0,
  battery: 0,
  battDisplayLevel: 0,
  batteryV: 0,
  charging: false,
  fullBatt: false,
  previousBattery: -1,  // para detectar cruce de umbrales
  btPower: false,
  battEffectActive: false,
  tempEffectActive: false,
  wifiRSSI: -70,
  deviceIP: '0.0.0.0',
  uptime: 0,

  // Random mode
  randomFXMode: false,
  randomVUMode: false,
  _randomFXTimer: null,
  _randomVUTimer: null,

  // Battery history (for chart) — populated from ESP32 LittleFS log on open
  battHistory: [],
  _battHistoryInitialized: false,

  // Client identity (WebSocket master/slave)
  clientId: 0,
  isMaster: false,
  _hasReceivedClientList: false,

  // UI
  currentTab: 'tabLamp',
  isDesktop: window.innerWidth >= 768,
  theme: localStorage.getItem('sml-theme') || 'sml-classic',

  // Volume (15 steps, 0-14, middle = 7)
  volumeLevel: 7,
  playing: false,

  // WebSocket reconnect state
  wsReconnectCount: 0,
  wsRetryDelay: 1000,
};

// ============================================================================
// DOM SHORTCUTS
// ============================================================================

const $ = (sel) => document.querySelector(sel);
const $$ = (sel) => document.querySelectorAll(sel);

// ============================================================================
// TAB SYSTEM
// ============================================================================

function initTabs() {
  // Set ARIA roles on nav containers
  const sidebar = document.getElementById('sidebar');
  const bottomNav = document.getElementById('bottomNav');
  if (sidebar) sidebar.setAttribute('role', 'tablist');
  if (bottomNav) bottomNav.setAttribute('role', 'tablist');

  const navBtns = $$('.nav-btn, .sidebar-btn');
  navBtns.forEach(btn => {
    btn.setAttribute('role', 'tab');
    btn.setAttribute('aria-selected', 'false');
    btn.setAttribute('tabindex', '-1');
    const tabId = btn.dataset.tab;
    if (tabId) {
      const panel = document.getElementById(tabId);
      if (panel) {
        btn.setAttribute('aria-controls', tabId);
        panel.setAttribute('role', 'tabpanel');
        panel.setAttribute('aria-labelledby', tabId + '-tab');
        btn.id = tabId + '-tab';
      }
    }
    btn.addEventListener('click', () => {
      if (tabId) switchTab(tabId);
    });
  });

  // Mark active tab
  const activeBtn = document.querySelector(`.nav-btn.active, .sidebar-btn.active`);
  if (activeBtn) activeBtn.setAttribute('aria-selected', 'true');

  // Keyboard navigation for tabs
  document.addEventListener('keydown', (e) => {
    if (!['ArrowLeft', 'ArrowRight', 'ArrowUp', 'ArrowDown', 'Home', 'End'].includes(e.key)) return;
    const tabs = Array.from(document.querySelectorAll('[role="tab"]'));
    if (tabs.length === 0) return;
    // Only handle if focus is inside a tablist
    const focused = document.activeElement;
    if (!focused || focused.getAttribute('role') !== 'tab') return;

    e.preventDefault();
    const currentIdx = tabs.indexOf(focused);
    let nextIdx = currentIdx;
    if (e.key === 'ArrowRight' || e.key === 'ArrowDown') nextIdx = (currentIdx + 1) % tabs.length;
    else if (e.key === 'ArrowLeft' || e.key === 'ArrowUp') nextIdx = (currentIdx - 1 + tabs.length) % tabs.length;
    else if (e.key === 'Home') nextIdx = 0;
    else if (e.key === 'End') nextIdx = tabs.length - 1;
    if (nextIdx !== currentIdx && tabs[nextIdx]) {
      const tabId = tabs[nextIdx].dataset.tab;
      if (tabId) switchTab(tabId);
      tabs[nextIdx].focus();
    }
  });
}

function switchTab(tabId) {
  $$('.nav-btn, .sidebar-btn').forEach(btn => {
    const isActive = btn.dataset.tab === tabId;
    btn.classList.toggle('active', isActive);
    btn.setAttribute('aria-selected', isActive ? 'true' : 'false');
    btn.setAttribute('tabindex', isActive ? '0' : '-1');
  });
  $$('.tab-content').forEach(tab => {
    tab.classList.toggle('active', tab.id === tabId);
  });
  SML.currentTab = tabId;

  // Dispatch custom event for config.js polling
  document.dispatchEvent(new CustomEvent('tabSwitch', { detail: { tab: tabId } }));

  // Stop peek render when leaving peek tab
  if (tabId !== 'tabPeek' && typeof peek !== 'undefined' && peek) {
    peek.stop();
    const peekToggle = document.getElementById('peekToggle');
    if (peekToggle) {
      peekToggle.textContent = '▶ Start';
      peekToggle.classList.remove('active');
    }
    // Stop ESP32 live stream
    if (typeof sendCmd === 'function') sendCmd({ lv: false });
  }

  // Init peek when tab activated
  if (tabId === 'tabPeek' && typeof initPeek === 'function') {
    initPeek();
  }
}

// ============================================================================
// STATUS BAR
// ============================================================================

function updateWiFiBars(rssi) {
  const bars = document.querySelectorAll('.wifi-bar');
  let level = 0;
  if (rssi > -50) level = 4;
  else if (rssi > -65) level = 3;
  else if (rssi > -80) level = 2;
  else if (rssi > -90) level = 1;

  const strength = ['', 'weak', 'fair', 'good', 'strong'];
  bars.forEach((bar, i) => {
    bar.className = 'wifi-bar';
    if (i < level) bar.classList.add(strength[level]);
  });
}

function updateBatteryBar(level, charging) {
  const fill = document.querySelector('.batt-fill');
  const percent = document.querySelector('.batt-percent');
  const iconContainer = document.querySelector('.batt-indicator');
  const chargingEl = document.querySelector('.charging-icon');
  if (!fill || !percent) return;

  const clamped = Math.max(0, Math.min(100, level));
  fill.style.width = clamped + '%';
  percent.textContent = clamped + '%';

  // Fill bar classes: high / medium / low / critical
  fill.className = 'batt-fill';
  if (clamped <= 15) fill.classList.add('critical');
  else if (clamped <= 20) fill.classList.add('low');
  else if (clamped <= 60) fill.classList.add('medium');
  else fill.classList.add('high');

  // Percent text color + animation
  percent.className = 'batt-percent';
  if (charging) {
    percent.classList.add('charging-text');
  } else if (clamped <= 15) {
    percent.classList.add('critical-text');
  } else if (clamped <= 20) {
    percent.classList.add('low-text');
  } else if (clamped <= 60) {
    percent.classList.add('medium-text');
  } else {
    percent.classList.add('high-text');
  }

  iconContainer?.classList.toggle('batt-charging', charging);
  iconContainer?.classList.toggle('charging', charging);
  if (chargingEl) {
    chargingEl.style.display = charging ? 'inline' : 'none';
    if (charging) chargingEl.classList.add('charging-glow');
    else chargingEl.classList.remove('charging-glow');
  }
}

// ============================================================================
// THEME
// ============================================================================

function initTheme() {
  document.documentElement.setAttribute('data-theme', SML.theme);
  $$('.theme-option').forEach(opt => {
    opt.classList.toggle('active', opt.dataset.theme === SML.theme);
  });
}

function setTheme(themeName) {
  SML.theme = themeName;
  localStorage.setItem('sml-theme', themeName);

  // Smooth theme transition: add class, change attr, remove after settle
  document.documentElement.classList.add('theme-transitioning');
  requestAnimationFrame(() => {
    document.documentElement.setAttribute('data-theme', themeName);
    $$('.theme-option').forEach(opt => {
      opt.classList.toggle('active', opt.dataset.theme === themeName);
    });
    // Remove after transition completes
    setTimeout(() => {
      document.documentElement.classList.remove('theme-transitioning');
    }, 600);
  });
}

// ============================================================================
// INIT
// ============================================================================

document.addEventListener('DOMContentLoaded', () => {
  // Theme
  initTheme();

  // Add skeleton loading to all data-bearing elements
  document.querySelectorAll('.stat-value, .info-value, #weatherHumVal, #sysUptime, #sysHeap, #sysRSSI, #sysVersion, #battPercentDetail, #battVoltageDetail, #battStatus, #battChargeDetail')
    .forEach(el => el.classList.add('skeleton'));

  // Tabs
  initTabs();

  // Lamp controls
  initLampControls();

  // Battery toggle (tap pill to toggle batt LED effect)
  const battToggle = document.getElementById('batteryToggle');
  if (battToggle) {
    battToggle.addEventListener('click', () => {
      if (!SML.powerOn) { showToast('Turn on the NeoPixel strip first', 'warning'); return; }
      if (SML.randomFXMode) stopRandomFX();
      if (SML.randomVUMode) stopRandomVU();
      sendCmd({ action: 'toggleBatt' });
    });
  }

  // Temperature toggle (tap thermometer to toggle temp LED effect)
  const tempToggle = document.getElementById('tempToggle');
  if (tempToggle) {
    tempToggle.addEventListener('click', () => {
      if (!SML.powerOn) { showToast('Turn on the NeoPixel strip first', 'warning'); return; }
      if (SML.randomFXMode) stopRandomFX();
      if (SML.randomVUMode) stopRandomVU();
      sendCmd({ action: 'toggleTemp' });
    });
  }

  // Volume + BT are now handled by player.js (IIFE auto-inits)
  // player.js handles: play/pause, skip, BT star-tab, volume ring drag

  // Close config panels (uses shared helper)
  document.querySelectorAll('.offcanvas-close').forEach(el => {
    el.addEventListener('click', closeEffectConfig);
  });
  const overlay = document.getElementById('offcanvasOverlay');
  if (overlay) overlay.addEventListener('click', closeEffectConfig);
  document.querySelectorAll('.param-sheet-close').forEach(el => {
    el.addEventListener('click', closeEffectConfig);
  });
  const modOv = document.getElementById('paramModalOverlay');
  if (modOv) modOv.addEventListener('click', closeEffectConfig);

  /* ── Modal helpers: About & Help on mobile ── */
  let modalTouchStartY = 0;
  let modalTouchCurrentY = 0;
  let modalIsDragging = false;

  function openModal(id) {
    const el = document.getElementById(id);
    if (!el) return;
    el.classList.add('open');
    const overlay = document.getElementById(id + 'Overlay');
    if (overlay) overlay.classList.add('open');
    document.body.style.overflow = 'hidden';

    // Populate body from tab content on first open
    const bodyId = id + 'Body';
    const body = document.getElementById(bodyId);
    if (body && !body.children.length) {
      const tabId = id === 'helpModal' ? 'tabHelp' : 'tabAbout';
      const src = document.getElementById(tabId);
      if (src) {
        const clone = src.cloneNode(true);
        clone.classList.remove('tab-content'); // remove hidden visibility
        clone.classList.remove('active');
        body.appendChild(clone);
      }
    }
  }
  function closeModal(id) {
    const el = document.getElementById(id);
    if (!el) return;
    el.classList.remove('open');
    const overlay = document.getElementById(id + 'Overlay');
    if (overlay) overlay.classList.remove('open');
    document.body.style.overflow = '';
    // Reset drag state
    modalIsDragging = false;
    if (el) el.style.transform = '';
  }

  document.getElementById('mobileHelpBtn')?.addEventListener('click', () => openModal('helpModal'));
  document.getElementById('mobileAboutBtn')?.addEventListener('click', () => openModal('aboutModal'));

  document.getElementById('helpModalClose')?.addEventListener('click', () => closeModal('helpModal'));
  document.getElementById('aboutModalClose')?.addEventListener('click', () => closeModal('aboutModal'));

  document.getElementById('helpModalOverlay')?.addEventListener('click', () => closeModal('helpModal'));
  document.getElementById('aboutModalOverlay')?.addEventListener('click', () => closeModal('aboutModal'));

  // Keyboard: Escape closes modals
  document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') {
      closeModal('helpModal');
      closeModal('aboutModal');
    }
  });

  // Also apply swipe-to-dismiss to the effect param sheet
  const paramSheet = document.getElementById('paramBottomSheet');
  if (paramSheet) {
    paramSheet.addEventListener('touchstart', (e) => {
      if (e.touches.length !== 1) return;
      modalTouchStartY = e.touches[0].clientY;
      modalTouchCurrentY = modalTouchStartY;
      modalIsDragging = false;
    }, { passive: true });
    paramSheet.addEventListener('touchmove', (e) => {
      if (e.touches.length !== 1) return;
      modalTouchCurrentY = e.touches[0].clientY;
      const dy = modalTouchCurrentY - modalTouchStartY;
      if (dy > 0) {
        if (!modalIsDragging && dy > 10) modalIsDragging = true;
        if (modalIsDragging) {
          paramSheet.style.transform = `translateY(${dy}px)`;
          paramSheet.style.transition = 'none';
        }
      }
    }, { passive: true });
    paramSheet.addEventListener('touchend', () => {
      if (!modalIsDragging) return;
      const dy = modalTouchCurrentY - modalTouchStartY;
      paramSheet.style.transition = '';
      if (dy > 100) {
        closeEffectConfig();
      } else {
        paramSheet.style.transform = '';
      }
      modalIsDragging = false;
    }, { passive: true });
  }

  // Peek canvas config gear — abre el panel del efecto actual
  document.getElementById('peekCanvasConfigBtn')?.addEventListener('click', () => {
    const activeCard = document.querySelector('.effect-card.active');
    if (!activeCard) return;
    const effId = parseInt(activeCard.dataset.effectId);
    showEffectConfig(effId, activeCard);
  });

  // Theme options — click to commit, hover to preview
  $$('.theme-option').forEach(opt => {
    opt.addEventListener('click', () => setTheme(opt.dataset.theme));
    opt.addEventListener('mouseenter', () => {
      document.documentElement.setAttribute('data-theme', opt.dataset.theme);
    });
    opt.addEventListener('mouseleave', () => {
      document.documentElement.setAttribute('data-theme', SML.theme);
    });
  });

  // Config save buttons
  const wifiBtn = document.getElementById('wifiSaveBtn');
  if (wifiBtn) wifiBtn.addEventListener('click', saveWiFiConfig);

  // Reboot
  const rebootBtn = document.getElementById('rebootBtn');
  if (rebootBtn) {
    rebootBtn.addEventListener('click', async () => {
      if (!SML.isMaster) { showToast('Only the master can reboot the device', 'info'); return; }
      const ok = await showConfirm({
        title: 'Reboot ESP32?',
        message: 'The connection will be lost for a few seconds while the device restarts.',
        confirmText: 'Reboot',
        icon: '<span class="fas fa-redo-alt" style="color:var(--accent-warning)"></span>',
      });
      if (ok) {
        showToast('Rebooting...', 'info');
        sendCmd({ action: 'reboot' });
      }
    });
  }
  // Factory Reset
  const factoryResetBtn = document.getElementById('factoryResetBtn');
  if (factoryResetBtn) {
    factoryResetBtn.addEventListener('click', async () => {
      if (!SML.isMaster) { showToast('Only the master can factory reset the device', 'info'); return; }
      const ok = await showConfirm({
        title: 'Factory Reset?',
        message: 'This will clear WiFi credentials, reset all effect parameters to defaults, and restart the device. You will need to re-connect to WiFi after this.',
        confirmText: 'Factory Reset',
        icon: '<span class="fas fa-exclamation-circle" style="color:var(--accent-danger)"></span>',
      });
      if (ok) {
        showToast('Factory resetting...', 'error');
        sendCmd({ action: 'factoryReset' });
      }
    });
  }

  // Enter key on WiFi pass
  const wifiPass = document.getElementById('wifiPass');
  if (wifiPass) {
    wifiPass.addEventListener('keydown', (e) => {
      if (e.key === 'Enter') saveWiFiConfig();
    });
  }

  // Battery chart button
  const battChartBtn = document.getElementById('batteryChartBtn');
  const battChartOverlay = document.getElementById('battChartOverlay');
  const battChartOffcanvas = document.getElementById('battChartOffcanvas');
  const battChartClose = document.getElementById('battChartClose');

  let _battChartRefreshTimer = null;

  function openBatteryChart() {
    if (!battChartOffcanvas || !battChartOverlay) return;
    // Request fresh history from ESP32 (sendCmd is no-op if WS not connected)
    if (typeof sendCmd === 'function') {
      sendCmd({ action: 'requestBattHistory' });
    }
    battChartOverlay.classList.add('open');
    battChartOffcanvas.classList.add('open');
    document.body.style.overflow = 'hidden';
    renderBatteryChart();
    // Auto-refresh every 10s while chart is open
    if (_battChartRefreshTimer) clearInterval(_battChartRefreshTimer);
    _battChartRefreshTimer = setInterval(function() {
      if (typeof sendCmd === 'function') {
        sendCmd({ action: 'requestBattHistory' });
      }
    }, 10000);
  }

  function closeBatteryChart() {
    if (!battChartOffcanvas || !battChartOverlay) return;
    battChartOverlay.classList.remove('open');
    battChartOffcanvas.classList.remove('open');
    document.body.style.overflow = '';
    if (_battChartRefreshTimer) {
      clearInterval(_battChartRefreshTimer);
      _battChartRefreshTimer = null;
    }
  }

  if (battChartBtn) battChartBtn.addEventListener('click', openBatteryChart);
  if (battChartOverlay) battChartOverlay.addEventListener('click', closeBatteryChart);
  if (battChartClose) battChartClose.addEventListener('click', closeBatteryChart);

  // Re-render chart when offcanvas opens (in case new data arrived)
  if (battChartOffcanvas) {
    const observer = new MutationObserver(() => {
      if (battChartOffcanvas.classList.contains('open')) {
        renderBatteryChart();
      }
    });
    observer.observe(battChartOffcanvas, { attributes: true, attributeFilter: ['class'] });
  }

  window.addEventListener('resize', handleResize);

  // Load effect metadata from server (caches into effectMetaCache)
  fetchFxdata();

  // Load palette data for selector
  fetchPalettes();

  // Connect WebSocket
  connectWS();

  // Activate initial tab
  switchTab(SML.currentTab);

  // Initial sync for Peek config button state
  updatePeekEffectInfo();

  // Notify preserved modules are ready
  if (typeof initBatteryAnimation === 'function') initBatteryAnimation();
});

// ── PWA Service Worker ───────────────────────────────────────────────────────
if ('serviceWorker' in navigator) {
  navigator.serviceWorker.register('/sw.js').then(reg => {
    console.debug('[PWA] ServiceWorker registered, scope:', reg.scope);
  }).catch(err => {
    console.warn('[PWA] ServiceWorker registration failed:', err);
  });
}
