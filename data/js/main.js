/* ──────────────────────────────────────────────────────────────────────────────
   main.js — SML Web UI (Phase 0 - protocol compatible)
   WebSocket handler + Tab routing + Theme system + UI controllers
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
  46: 'Gravimeter VU', 47: 'Noisemeter VU', 48: 'DJ Light VU', 49: 'PS1 DGEQ VU',
  50: 'Palette Blend VU',
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
    }, 400);
  });
}

// ============================================================================
// LAMP CONTROLS
// ============================================================================

function togglePowerCard(el, isOn) {
  el.classList.toggle('on', isOn);
}

function initLampControls() {
  // Lamp toggle
  const lampToggle = document.getElementById('lampToggle');
  if (lampToggle) {
    lampToggle.addEventListener('click', () => {
      sendCmd({ action: 'lamp' });
      togglePowerCard(lampToggle, !lampToggle.classList.contains('on'));
    });
  }

  // Neopixel toggle
  const neoToggle = document.getElementById('neoToggle');
  if (neoToggle) {
    neoToggle.addEventListener('click', () => {
      sendCmd({ action: 'toggle' });
      SML.powerOn = !SML.powerOn;
      togglePowerCard(neoToggle, SML.powerOn);
      updatePeekEffectInfo();
      if (!SML.powerOn) {
        if (SML.randomFXMode) { stopRandomFX(); sendCmd({ action: 'randomFX', state: false }); }
        if (SML.randomVUMode) { stopRandomVU(); sendCmd({ action: 'randomVU', state: false }); }
      }
    });
  }

  // ── Color picker (iro.js) — wheel ONLY, sin value slider ─────────────
  // El brillo tiene su propio slider HTML aparte para evitar
  // enviar brillo+cada vez que se cambia color (race condition).
  if (typeof iro !== 'undefined' && document.getElementById('colorPicker')) {
    const colorPicker = new iro.ColorPicker('#colorPicker', {
      width: 220,
      color: { h: 0, s: 0, v: 51 },  // V fijo, no controla brillo real
      borderWidth: 0,
      handleRadius: 8,
      layout: [
        { component: iro.ui.Wheel },
        // NOTA: sin Slider value — el brillo va en slider HTML separado
      ],
    });

    SML._colorRemoteLock = false;
    SML._colorDragging = false;
    colorPicker.on('input:start', () => {
      SML._colorDragging = true;
    });
    colorPicker.on('color:change', (color) => {
      if (SML._colorRemoteLock) return;
      SML.r = color.rgb.r;
      SML.g = color.rgb.g;
      SML.b = color.rgb.b;
      // NOTA: no tocamos SML.brightness aquí — el brillo es independiente
      updateSolidIcon(SML.r, SML.g, SML.b);
    });

    colorPicker.on('input:end', () => {
      SML._colorDragging = false;
      SML._lastColorSent = Date.now(); // para ignorar broadcasts stale
      // Solo color, sin brightness — mensaje limpio
      sendCmd({ action: 'picker', color: { r: SML.r, g: SML.g, b: SML.b } });
    });

    SML.colorPicker = colorPicker;
  }

  // ── Global brightness FAB + pill slider (battery-fill style) ──────────
  const fabBtn = document.getElementById('fabBtn');
  const fabSlider = document.getElementById('fabSlider');
  const fabSliderFill = document.getElementById('fabSliderFill');
  const fabSliderContainer = document.getElementById('fabSliderContainer');
  const fabValueEl = document.getElementById('fabValue');
  let _fabHideTimer = null;
  let _fabDragging = false;
  const BRIGHT_MAX = 255;

  function updateFabSliderFill(v) {
    if (!fabSliderFill) return;
    const pct = Math.round((v / BRIGHT_MAX) * 100);
    fabSliderFill.style.height = pct + '%';
    if (fabValueEl) fabValueEl.textContent = v;
  }

  function valueFromClientY(clientY) {
    const rect = fabSlider.getBoundingClientRect();
    // 0 at bottom, 255 at top
    const y = rect.bottom - clientY;
    const normalized = Math.max(0, Math.min(1, y / rect.height));
    return Math.round(normalized * BRIGHT_MAX);
  }

  function onFabSliderPointerDown(e) {
    const clientY = e.touches ? e.touches[0].clientY : e.clientY;
    const v = valueFromClientY(clientY);
    SML.brightness = v;
    updateFabSliderFill(v);
    _fabDragging = true;
    if (_fabHideTimer) clearTimeout(_fabHideTimer);
    e.preventDefault();
  }

  function onFabSliderPointerMove(e) {
    if (!_fabDragging) return;
    const clientY = e.touches ? e.touches[0].clientY : e.clientY;
    const v = valueFromClientY(clientY);
    SML.brightness = v;
    updateFabSliderFill(v);
    e.preventDefault();
  }

  function onFabSliderPointerUp(e) {
    if (!_fabDragging) return;
    _fabDragging = false;
    SML._lastBrightnessSent = Date.now();
    sendCmd({ action: 'slider', brightness: SML.brightness });
    if (_fabHideTimer) clearTimeout(_fabHideTimer);
    _fabHideTimer = setTimeout(() => {
      hideFabSlider();
    }, 3000);
  }

  function showFabSlider() {
    fabSliderContainer.classList.add('active');
    document.getElementById('brightnessFab').classList.add('slider-open');
    if (_fabHideTimer) clearTimeout(_fabHideTimer);
    _fabHideTimer = setTimeout(() => {
      hideFabSlider();
    }, 5000);
  }

  function hideFabSlider() {
    fabSliderContainer.classList.remove('active');
    document.getElementById('brightnessFab').classList.remove('slider-open');
    if (_fabHideTimer) { clearTimeout(_fabHideTimer); _fabHideTimer = null; }
  }

  // Toggle FAB on click
  if (fabBtn && fabSliderContainer) {
    fabBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      if (fabSliderContainer.classList.contains('active')) {
        hideFabSlider();
      } else {
        showFabSlider();
      }
    });
  }

  // Custom slider pointer events
  if (fabSlider) {
    fabSlider.addEventListener('mousedown', onFabSliderPointerDown);
    fabSlider.addEventListener('touchstart', onFabSliderPointerDown, { passive: false });
    document.addEventListener('mousemove', onFabSliderPointerMove);
    document.addEventListener('touchmove', onFabSliderPointerMove, { passive: false });
    document.addEventListener('mouseup', onFabSliderPointerUp);
    document.addEventListener('touchend', onFabSliderPointerUp);
    document.addEventListener('touchcancel', onFabSliderPointerUp);
  }

  // Init fill from current brightness value
  SML._initFabBrightness = function (v) {
    updateFabSliderFill(v);
  };
  SML._initFabBrightness(SML.brightness);

  // Click outside FAB to close
  document.addEventListener('click', (e) => {
    const fab = document.getElementById('brightnessFab');
    if (fab && !fab.contains(e.target) && fabSliderContainer?.classList.contains('active')) {
      hideFabSlider();
    }
  });

  // Effect cards
  initEffectCards();
}

// ── Solid icon: refleja el color del picker solo si está activo ─────
function updateSolidIcon(r, g, b) {
  const card = document.querySelector('.effect-card[data-effect-id="0"]');
  const el = card?.querySelector('.effect-icon');
  if (!el) return;
  if (card.classList.contains('active')) {
    el.style.setProperty('--solid-fill', `rgb(${r},${g},${b})`);
  } else {
    el.style.removeProperty('--solid-fill');
  }
}

function initEffectCards() {
  const cards = $$('.effect-card');
  let activeCard = null;

  cards.forEach(card => {
    card.addEventListener('click', () => {
      const effId = parseInt(card.dataset.effectId);
      if (isNaN(effId)) return;

      // Toast warning if NeoPixel is off
      if (!SML.powerOn) {
        showToast('Turn on the NeoPixel strip first', 'warning');
        return;
      }

      // wasActive check — moved here BEFORE early returns for Random cards
      const wasActive = card.classList.contains('active');

      // ── Random FX (ID 99) ──
      if (effId === 99) {
        handleRandomFXClick(card, wasActive);
        return;
      }
      // ── Random VU (ID 100) ──
      if (effId === 100) {
        handleRandomVUClick(card, wasActive);
        return;
      }

      // ── Normal effect cards ──
      // Si el efecto ya está activo y no estamos en random mode,
      // solo abrir configuración sin enviar comando redundante al ESP32
      if (wasActive && !SML.randomFXMode && !SML.randomVUMode) {
        closeEffectConfig();
        showEffectConfig(effId, card);
        return;
      }

      // Si random estaba activo, detenerlo (backend + frontend)
      if (SML.randomFXMode) {
        stopRandomFX();
        sendCmd({ action: 'randomFX', state: false, effectId: effId });
      } else if (SML.randomVUMode) {
        stopRandomVU();
        sendCmd({ action: 'randomVU', state: false, effectId: effId });
      } else {
        // Enviar comando de efecto solo (sin stop random)
        sendCmd({ effectId: effId });
      }

      // Si cambia a otro efecto, cerrar config abierta
      if (activeCard && activeCard !== card) {
        closeEffectConfig();
      }

      // Actualizar estado visual
      SML.effectId = effId;
      cards.forEach(c => c.classList.remove('active'));
      card.classList.add('active');
      activeCard = card;
      updateSolidIcon(SML.r, SML.g, SML.b);
      updatePeekEffectInfo();

      // Segundo click en la misma card → mostrar config
      if (wasActive) {
        showEffectConfig(effId, card);
      }
    });
  });
}

// ============================================================================
// RANDOM FX MODE — cycles through all non-VU effects
// ============================================================================

const RANDOM_FX_POOL = [
  1,2,3,4,5,6,7,8,9,10,11,     // Fire → ColorSweep
  20,21,22,23,24,25,26,27,      // ColorWipe → Sparkle
  28,29,30,31,32,33,34,35,36,37,// BPM → Popcorn
  38,39,40,41,42,43,44,45       // LarsonScanner → HallowEyes
];

const RANDOM_VU_POOL = [
  12,13,14,15,16,17,            // RainbowVU → OceanVU
  46,47,48,49,                  // Gravimeter → PS1DGEQ
  50,                           // Palette Blend
];

function getRandomDuration() {
  const val = parseInt(localStorage.getItem('sml-random-duration') || '8');
  return Math.max(3000, val * 1000);
}

function handleRandomFXClick(card, wasActive) {
  const cards = $$('.effect-card');

  if (wasActive) {
    // Segundo click → mostrar config de duración
    closeEffectConfig();
    showEffectConfig(99, card);
    return;
  }

  if (SML.randomFXMode) {
    // Ya en random pero wasActive=false (remoto) → detener localmente
    stopRandomFX();
    sendCmd({ action: 'randomFX', state: false });
    // Volver a Solid
    SML.effectId = 0;
    sendCmd({ effectId: 0 });
    cards.forEach(c => c.classList.remove('active'));
    updateSolidIcon(SML.r, SML.g, SML.b);
    updatePeekEffectInfo();
  } else {
    // Detener VU random si estaba activo
    if (SML.randomVUMode) {
      stopRandomVU();
      sendCmd({ action: 'randomVU', state: false });
    }

    // Iniciar random FX (avisa al backend)
    sendCmd({ action: 'randomFX', state: true });
    SML.randomFXMode = true;
    cards.forEach(c => c.classList.remove('active'));
    card.classList.add('active');
    updatePeekEffectInfo();
    cycleRandomFX();
  }
}

function handleRandomVUClick(card, wasActive) {
  const cards = $$('.effect-card');

  if (wasActive) {
    // Segundo click → mostrar config de duración
    closeEffectConfig();
    showEffectConfig(100, card);
    return;
  }

  if (SML.randomVUMode) {
    // Ya en random pero wasActive=false (remoto) → detener localmente
    stopRandomVU();
    sendCmd({ action: 'randomVU', state: false });
    // Volver a Solid
    SML.effectId = 0;
    sendCmd({ effectId: 0 });
    cards.forEach(c => c.classList.remove('active'));
    updateSolidIcon(SML.r, SML.g, SML.b);
    updatePeekEffectInfo();
  } else {
    // Detener FX random si estaba activo
    if (SML.randomFXMode) {
      stopRandomFX();
      sendCmd({ action: 'randomFX', state: false });
    }

    // Iniciar random VU (avisa al backend)
    sendCmd({ action: 'randomVU', state: true });
    SML.randomVUMode = true;
    cards.forEach(c => c.classList.remove('active'));
    card.classList.add('active');
    updatePeekEffectInfo();
    cycleRandomVU();
  }
}

function cycleRandomFX() {
  if (!SML.randomFXMode) return;
  const pool = RANDOM_FX_POOL;
  const id = pool[Math.floor(Math.random() * pool.length)];
  SML.effectId = id;
  sendCmd({ effectId: id });
  // No marcar ninguna card como activa excepto la Random FX
  // (cada efecto se ve solo por el tiempo que dure)
  SML._randomFXTimer = setTimeout(cycleRandomFX, getRandomDuration());
}

function cycleRandomVU() {
  if (!SML.randomVUMode) return;
  const pool = RANDOM_VU_POOL;
  const id = pool[Math.floor(Math.random() * pool.length)];
  SML.effectId = id;
  sendCmd({ effectId: id });
  SML._randomVUTimer = setTimeout(cycleRandomVU, getRandomDuration());
}

function stopRandomFX() {
  SML.randomFXMode = false;
  if (SML._randomFXTimer) {
    clearTimeout(SML._randomFXTimer);
    SML._randomFXTimer = null;
  }
}

function stopRandomVU() {
  SML.randomVUMode = false;
  if (SML._randomVUTimer) {
    clearTimeout(SML._randomVUTimer);
    SML._randomVUTimer = null;
  }
}

function closeEffectConfig() {
  const offcanvas = document.getElementById('effectOffcanvas');
  const sheet = document.getElementById('paramBottomSheet');
  const ocOverlay = document.getElementById('offcanvasOverlay');
  const modOv = document.getElementById('paramModalOverlay');
  if (offcanvas) offcanvas.classList.remove('open');
  if (sheet) sheet.classList.remove('open');
  if (ocOverlay) ocOverlay.classList.remove('open');
  if (modOv) modOv.classList.remove('open');
}

/**
 * Update the Peek canvas overlay with the currently active effect name.
 * Shows "—" when NeoPixel is off or no effect is active.
 */
function updatePeekEffectInfo() {
  const nameEl = document.getElementById('peekCanvasEffectName');
  const btn = document.getElementById('peekCanvasConfigBtn');
  if (!nameEl || !btn) return;

  const activeCard = document.querySelector('.effect-card.active');
  if (activeCard && SML.powerOn) {
    const name = activeCard.querySelector('.effect-name')?.textContent || 'Effect';
    nameEl.textContent = name;
    nameEl.classList.add('has-effect');
    btn.title = 'Configure ' + name;
    btn.style.display = 'flex';
  } else {
    nameEl.textContent = '—';
    nameEl.classList.remove('has-effect');
    btn.title = 'Turn on NeoPixel to configure';
    btn.style.display = 'none';
  }
}

function showEffectConfig(effId, cardEl) {
  // Special: Random FX/VU — show duration config inline
  if (effId === 99 || effId === 100) {
    showRandomDurationConfig(effId, cardEl);
    return;
  }

  if (window.innerWidth < 768) {
    // Mobile: bottom sheet modal
    const sheet = document.getElementById('paramBottomSheet');
    const body = document.getElementById('paramSheetBody');
    const title = document.getElementById('paramSheetTitle');
    const overlay = document.getElementById('paramModalOverlay');
    if (sheet && body && title) {
      const nameEl = cardEl.querySelector('.effect-name');
      title.textContent = nameEl ? nameEl.textContent : 'Effect';
      renderEffectParams(effId, body);
      sheet.classList.add('open');
      if (overlay) overlay.classList.add('open');
    }
  } else {
    // Desktop: offcanvas
    const offcanvas = document.getElementById('effectOffcanvas');
    const body = document.getElementById('effectOffcanvasBody');
    const title = document.getElementById('effectOffcanvasTitle');
    if (offcanvas && body && title) {
      const nameEl = cardEl.querySelector('.effect-name');
      title.textContent = nameEl ? nameEl.textContent : 'Effect';
      renderEffectParams(effId, body);
      offcanvas.classList.add('open');
      const overlay = document.getElementById('offcanvasOverlay');
      if (overlay) overlay.classList.add('open');
    }
  }
}

function showRandomDurationConfig(effId, cardEl) {
  const label = effId === 99 ? 'Random FX' : 'Random VU';
  const body = window.innerWidth < 768
    ? document.getElementById('paramSheetBody')
    : document.getElementById('effectOffcanvasBody');
  const title = window.innerWidth < 768
    ? document.getElementById('paramSheetTitle')
    : document.getElementById('effectOffcanvasTitle');
  const overlay = window.innerWidth < 768
    ? document.getElementById('paramModalOverlay')
    : document.getElementById('offcanvasOverlay');
  const container = window.innerWidth < 768
    ? document.getElementById('paramBottomSheet')
    : document.getElementById('effectOffcanvas');

  if (!body || !title || !container) return;
  title.textContent = label;
  const curVal = localStorage.getItem('sml-random-duration') || '8';
  body.innerHTML = `
    <div class="param-row">
      <label>Duration (seconds)</label>
      <input type="range" id="randomDurationInline" min="3" max="30" value="${curVal}" step="1">
      <span class="param-value" id="randomDurationInlineVal">${curVal}s</span>
    </div>
    <div class="palette-section-title" style="margin-top:12px"><span class="fas fa-info-circle"></span> About</div>
    <p style="font-size:0.8rem;color:var(--text-secondary);padding:8px;line-height:1.5">
      Each effect plays for the set duration, then randomly switches to the next one.
      Effects use their default parameters.
    </p>
  `;
  const slider = body.querySelector('#randomDurationInline');
  const valEl = body.querySelector('#randomDurationInlineVal');
  if (slider && valEl) {
    slider.addEventListener('input', () => {
      valEl.textContent = slider.value + 's';
      localStorage.setItem('sml-random-duration', slider.value);
    });
  }

  container.classList.add('open');
  if (overlay) overlay.classList.add('open');
}

function renderEffectParams(effId, container) {
  const config = effectMetaCache[effId];
  const paletteData = window._paletteData || { names: [], swatches: [] };
  const live = liveEffectParams[effId] || {};
  const currentPalette = live.palette !== undefined ? live.palette : 0;

  // Sin parámetros sliders, solo mostrar el selector de paletas
  if (!config || !config.params || config.params.length === 0) {
    container.innerHTML = renderPaletteSection(effId, paletteData, currentPalette);
    attachPaletteHandlers(effId, container);
    return;
  }

  container.innerHTML = config.params.map(p => {
    const liveVal = live[p.key] !== undefined ? live[p.key] : p.default;
    if (p.type === 'checkbox') {
      return `
        <div class="param-row">
          <label>${p.label}</label>
          <label class="switch">
            <input type="checkbox" data-key="${p.key}" ${liveVal ? 'checked' : ''}>
            <span class="slider"></span>
          </label>
        </div>`;
    }
    return `
      <div class="param-row">
        <label>${p.label}</label>
        <input type="range" min="${p.min}" max="${p.max}" value="${liveVal}"
               data-key="${p.key}">
        <span class="param-value">${liveVal}</span>
      </div>`;
  }).join('') + renderPaletteSection(effId, paletteData, currentPalette) + `
    <div class="param-reset-row">
      <button class="btn-reset-params" data-effect-id="${effId}">↺ Reset to Defaults</button>
    </div>`;

  // Sliders — envía en change (al soltar) para no saturar al ESP32 con
  // cientos de writes a LittleFS durante el arrastre. El guard
  // document.activeElement en el sync evita que notifyClients() overwrite.
  container.querySelectorAll('input[type="range"]').forEach(input => {
    const valSpan = input.nextElementSibling;
    input.addEventListener('input', () => { valSpan.textContent = input.value; });
    input.addEventListener('change', () => {
      const msg = { action: 'setParams', effectId: effId };
      msg[input.dataset.key] = parseInt(input.value);
      sendCmd(msg);
    });
  });

  // Checkboxes
  container.querySelectorAll('input[type="checkbox"]').forEach(input => {
    input.addEventListener('change', () => {
      const newVal = input.checked ? 1 : 0;
      console.debug(`[DEBUG] Checkbox ${input.dataset.key} → ${newVal} (input.checked=${input.checked})`);
      const msg = { action: 'setParams', effectId: effId };
      msg[input.dataset.key] = newVal;
      sendCmd(msg);
    });
  });

  // Reset to defaults button
  const resetBtn = container.querySelector('.btn-reset-params');
  if (resetBtn) {
    resetBtn.addEventListener('click', () => {
      // Limpiar cache de valores activos para que al reabrir
      // se vean los defaults reales, no valores stale
      delete liveEffectParams[effId];
      const cfg = effectMetaCache[effId];
      if (cfg && cfg.params) {
        const msg = { action: 'setParams', effectId: effId };
        cfg.params.forEach(p => {
          msg[p.key] = p.default;
          const input = container.querySelector(`input[data-key="${p.key}"]`);
          if (input) {
            if (input.type === 'checkbox') {
              input.checked = !!p.default;
            } else {
              input.value = p.default;
              const valSpan = input.nextElementSibling;
              if (valSpan) valSpan.textContent = p.default;
            }
          }
        });
        // Include default palette in the same message
        if (cfg.defaultPalette !== undefined) {
          msg.palette = cfg.defaultPalette;
        }
        sendCmd(msg);

        // Update palette visual selection
        if (cfg.defaultPalette !== undefined) {
          container.querySelectorAll('.palette-swatch').forEach(s => {
            const idx = parseInt(s.dataset.paletteIndex);
            s.classList.toggle('selected', idx === cfg.defaultPalette);
            s.querySelector('.palette-check-mark').textContent = idx === cfg.defaultPalette ? '✓' : '';
          });
        }
      }
    });
  }

  // Palette click handlers
  attachPaletteHandlers(effId, container);
}

// ── Palette section HTML ──────────────────────────────────────────────────────
function renderPaletteSection(effId, paletteData, currentPalette) {
  if (!paletteData.names || paletteData.names.length === 0) {
    return '<div class="palette-section"><p class="text-muted" style="font-size:0.75rem;padding:8px 0">Loading palettes...</p></div>';
  }

  // Get this effect's default palette index from meta cache
  const cfg = effectMetaCache[effId];
  const effectDefPal = cfg ? cfg.defaultPalette : 0;

  // Build display order: effect's own default first (if special > 17),
  // then regular palettes (0-17). No mostrar defaults de otros efectos.
  const displayIndices = [];
  if (effectDefPal > 17) {
    displayIndices.push(effectDefPal);
  }
  for (let i = 0; i <= 17; i++) {
    displayIndices.push(i);
  }

  let html = '<div class="palette-section">';
  html += '<div class="palette-section-title"><span class="fas fa-palette"></span> Color Palette</div>';
  html += '<div class="palette-grid">';

  displayIndices.forEach(i => {
    if (i >= paletteData.names.length) return;
    const selected = i === currentPalette ? ' selected' : '';
    const swatchColors = paletteData.swatches[i] || [];

    html += `<div class="palette-swatch${selected}" data-palette-index="${i}">`;
    html += '<div class="palette-swatch-bar">';
    for (let j = 0; j < 6 && j * 3 < swatchColors.length; j++) {
      const r = swatchColors[j * 3];
      const g = swatchColors[j * 3 + 1];
      const b = swatchColors[j * 3 + 2];
      html += `<span class="palette-color" style="background:rgb(${r},${g},${b})"></span>`;
    }
    html += '</div>';
    html += `<span class="palette-name">${paletteData.names[i]}</span>`;
    html += `<div class="palette-check-mark">${selected ? '✓' : ''}</div>`;
    html += '</div>';
  });

  html += '</div></div>';
  return html;
}

function attachPaletteHandlers(effId, container) {
  container.querySelectorAll('.palette-swatch').forEach(el => {
    el.addEventListener('click', () => {
      const idx = parseInt(el.dataset.paletteIndex);
      const msg = { action: 'setParams', effectId: effId };
      msg.palette = idx;
      sendCmd(msg);
      // Update visual selection
      container.querySelectorAll('.palette-swatch').forEach(s => s.classList.remove('selected'));
      el.classList.add('selected');
      el.querySelector('.palette-check-mark').textContent = '✓';
    });
  });
}

// ── Fetch palette data ────────────────────────────────────────────────────────
async function fetchPalettes() {
  try {
    const resp = await fetch('/palettes');
    const data = await resp.json();
    window._paletteData = data;
  } catch (e) {
    // Silencioso — reintenta en el próximo render
  }
}

// ============================================================================
// EFFECT METADATA SYSTEM (WLED-style)
// ============================================================================
// Almacena la metadata parseada de cada efecto, recibida desde el ESP32
// vía HTTP /fxdata. Es la ÚNICA fuente de verdad para labels y defaults.
// ============================================================================

let effectMetaCache = {};  // { [effectId]: { name, params } }

// Cache de valores activos de parámetros recibidos vía WebSocket.
// { [effectId]: { speed: 120, intensity: 50, ... } }
// renderEffectParams() prioriza estos valores sobre effectMetaCache[].default
// para que los sliders reflejen el estado real del ESP32 al abrir la config.
let liveEffectParams = {};

/**
 * Parsea una cadena de metadatos estilo WLED y retorna { name, params[] }.
 * Formato: "Name@label_speed,label_intensity,label_c1,...,label_m3;;;;sx=64,ix=128,c1=55,..."
 * Labels vacías = sin slider para ese parámetro.
 */
function parseEffectMeta(metaStr) {
  const atIdx = metaStr.indexOf('@');
  if (atIdx < 0) return null;
  const name = metaStr.substring(0, atIdx);
  const rest = metaStr.substring(atIdx + 1);

  // Defaults section: after last ';'
  const lastSemi = rest.lastIndexOf(';');
  const mainPart = lastSemi >= 0 ? rest.substring(0, lastSemi) : rest;
  const defaultsPart = lastSemi >= 0 ? rest.substring(lastSemi + 1) : '';

  // Labels section: first segment (before first ';')
  const firstSemi = mainPart.indexOf(';');
  const labelsStr = firstSemi >= 0 ? mainPart.substring(0, firstSemi) : mainPart;
  const labels = labelsStr.split(',');

  // Parse key=value defaults
  const defaults = {};
  if (defaultsPart) {
    defaultsPart.split(',').forEach(pair => {
      pair = pair.trim();
      if (!pair) return;
      const eq = pair.indexOf('=');
      if (eq < 0) return;
      defaults[pair.substring(0, eq).trim()] = pair.substring(eq + 1).trim();
    });
  }

  // Map label positions to param keys and their default keys
  // Position: 0=speed, 1=intensity, 2=c1, 3=c2, 4=c3, 5=reserved, 6=m1, 7=m2, 8=m3
  const paramKeys   = ['speed', 'intensity', 'custom1', 'custom2', 'custom3', null, 'check1', 'check2', 'check3'];
  const defaultKeys = ['sx',    'ix',        'c1',      'c2',      'c3',      null, 'm1',      'm2',      'm3'];

  const params = [];
  labels.forEach((label, i) => {
    label = label.trim();
    if (!label || !paramKeys[i]) return;

    // Parse optional range from label: "Label:min:max" → { displayLabel, min, max }
    let parts = label.split(':');
    let displayLabel = parts[0];
    let min = 0, max = 255;
    if (parts.length === 3) {
      min = parseInt(parts[1]) || 0;
      max = parseInt(parts[2]) || 255;
    }

    const isBool = paramKeys[i].startsWith('check');
    const dk = defaultKeys[i];
    let defVal;

    if (isBool) {
      defVal = (dk && defaults[dk] !== undefined) ? (parseInt(defaults[dk]) !== 0) : false;
    } else {
      defVal = (dk && defaults[dk] !== undefined) ? parseInt(defaults[dk]) : 128;
    }

    params.push({
      key: paramKeys[i],
      label: displayLabel,
      type: isBool ? 'checkbox' : 'range',
      min: isBool ? 0 : min,
      max: isBool ? 1 : max,
      default: defVal
    });
  });

  return { name, params, defaultPalette: defaults.pa ? parseInt(defaults.pa) : 0 };
}

// ============================================================================
// FETCH FX DATA (metadata de efectos vía HTTP)
// ============================================================================
// Carga la metadata de todos los efectos desde el ESP32 via HTTP GET /fxdata.
// Esto reemplaza el envío masivo por WebSocket (Opción B).
// ============================================================================

async function fetchFxdata() {
  try {
    const resp = await fetch('/fxdata');
    const data = await resp.json();
    // data = { "1": "Fire@...", "2": "MovingDot@...", ... }
    Object.entries(data).forEach(([id, metaStr]) => {
      const parsed = parseEffectMeta(metaStr);
      if (parsed) effectMetaCache[parseInt(id)] = parsed;
    });
  } catch (e) {
    // Silencioso — sin fallback, la UI espera al próximo fetch
  }
}

// ============================================================================
// WEBSOCKET
// ============================================================================

function connectWS() {
  const protocol = location.protocol === 'https:' ? 'wss:' : 'ws:';
  const url = `${protocol}//${location.host}/ws`;

  if (SML.ws && SML.ws.readyState === WebSocket.OPEN) return;

  try {
    SML.ws = new WebSocket(url);
    SML.ws.binaryType = 'arraybuffer';
  } catch (e) {
    console.error('WS connection failed:', e);
    scheduleReconnect();
    return;
  }

  // Init retry backoff if first connection attempt
  if (SML.wsRetryDelay === undefined) SML.wsRetryDelay = 1000;

  // Expose globals for player.js
  websocket = SML.ws;

  SML.ws.onopen = () => {
    SML.connected = true;
    SML.wsReconnectCount = 0;
    SML.wsRetryDelay = 1000;          // Reset backoff on successful connect
    updateConnectionStatus(true);
    clearTimeout(SML.wsReconnectTimer);
    // Remove skeletons when connected
    document.querySelectorAll('.skeleton').forEach(el => el.classList.remove('skeleton'));
  };

  SML.ws.onclose = (evt) => {
    SML.connected = false;
    updateConnectionStatus(false);
    // Re-add skeleton to data elements when disconnected
    document.querySelectorAll('.stat-value, .info-value, #weatherHumVal, #weatherHeatIndex, #sysUptime, #sysHeap, #sysRSSI, #sysVersion, #deviceIP, #battPercentDetail, #battVoltageDetail, #battStatus, #battChargeDetail')
      .forEach(el => {
        if (el.textContent === '--' || el.textContent === '--.-' || el.textContent === '--.--V' || el.textContent === '--%') {
          el.classList.add('skeleton');
        }
      });
    // Log close code for debugging
    if (evt.code !== 1000 && evt.code !== 1001) {
      console.debug(`[WS] closed (${evt.code}), reconnecting...`);
    }
    scheduleReconnect();
  };

  SML.ws.onerror = (evt) => {
    // onerror is always followed by onclose — log and let close trigger reconnect
    console.warn('[WS] connection error', evt instanceof Event ? evt.type : evt);
  };

  SML.ws.onmessage = (event) => {
    // Binary data for Peek (real-time LED stream from ESP32)
    if (event.data instanceof ArrayBuffer) {
      if (typeof handlePeekBinary === 'function') handlePeekBinary(event.data);
      return;
    }

    try {
      const data = JSON.parse(event.data);
      handleMessage(data);
    } catch (e) {
      // Ignore non-JSON
    }
  };
}

function scheduleReconnect() {
  clearTimeout(SML.wsReconnectTimer);

  // Exponential backoff: 1s → 2s → 4s → 8s → 16s → 30s (cap)
  const delay = SML.wsRetryDelay || 1000;
  SML.wsReconnectCount = (SML.wsReconnectCount || 0) + 1;
  SML.wsRetryDelay = Math.min(delay * 1.5, 30000);

  console.debug(`[WS] reconnect #${SML.wsReconnectCount} in ${delay}ms`);
  SML.wsReconnectTimer = setTimeout(connectWS, delay);
}

function sendCmd(obj) {
  if (SML.ws && SML.ws.readyState === WebSocket.OPEN) {
    SML.ws.send(JSON.stringify(obj));
  }
}

function handleMessage(data) {
  // ── CONSOLE DEBUG — valores que manda el ESP32 ──
  console.debug('[SML ← ESP32]', JSON.stringify(data));

  // ── TEMPERATURE / HUMIDITY ──
  if (data.temperature !== undefined) {
    SML.temp = data.temperature;

    // Update thermometer animation
    const value = parseFloat(data.temperature);
    const tempEl = document.getElementById('temperature');
    if (tempEl) {
      const minTemp = 0, maxTemp = 50;
      const pct = Math.min(100, Math.max(0, ((value - minTemp) / (maxTemp - minTemp)) * 100));
      tempEl.style.height = pct + '%';
      tempEl.dataset.value = value.toFixed(1) + '°C';
    }

    // Weather display
    setDataValue(document.getElementById('weatherTempVal'), value.toFixed(1));
  }

  if (data.humidity !== undefined) {
    SML.hum = data.humidity;
    setDataValue(document.getElementById('weatherHumVal'), Math.round(data.humidity));
  }

  // ── BATTERY ──
  if (data.level !== undefined) {
    SML.battery = data.level;
    batt.level = data.level;

    // Smart capping: la librería da 100% al llegar a MAXV (4.0V), pero el
    // TP4056 aún no ha terminado la carga CV. Mostramos 99% hasta que
    // fullBatt confirme que la batería está realmente llena.
    const isCharging = data.charging !== undefined ? data.charging : !!SML.charging;
    const isFullBatt = data.fullbatt !== undefined ? data.fullbatt : !!SML.fullBatt;
    const displayLevel = (isCharging && !isFullBatt && data.level >= 99) ? 99 : data.level;

    SML.battDisplayLevel = displayLevel;
    updateBatteryBar(displayLevel, isCharging);
  }
  if (data.battVoltage !== undefined) {
    SML.batteryV = data.battVoltage;
    setDataValue(document.getElementById('battVoltageDetail'), data.battVoltage.toFixed(2), 'V');
  }
  if (data.charging !== undefined) {
    SML.charging = data.charging;
    batt.charging = data.charging;
  }
  if (data.fullbatt !== undefined) {
    SML.fullBatt = data.fullbatt;
    batt.fullbatt = data.fullbatt;
  }

  // Update battery.js visualization
  if (data.level !== undefined || data.charging !== undefined || data.fullbatt !== undefined) {
    if (typeof initBattery === 'function') initBattery(batt);
  }

  // ── Battery detail stats ──
  const prevLevel = SML.previousBattery;       // raw level anterior
  const rawLevel  = SML.battery || 0;           // raw level actual (sin cap)
  const dispLevel = SML.battDisplayLevel ?? rawLevel;  // con smart capping
  SML.previousBattery = rawLevel;

  // Flash al llegar a 100% real (fullBatt o rawLevel>=100 sin cap)
  const bpd = document.getElementById('battPercentDetail');
  if (bpd) {
    setDataValue(bpd, dispLevel, '%');
    if (rawLevel >= 100 && (prevLevel < 100 || prevLevel === undefined || prevLevel === -1)) {
      bpd.classList.add('flash');
      setTimeout(() => bpd.classList.remove('flash'), 3000);
    }
  }

  // ── STATUS + CHARGE — iconos puros (sin texto) ──
  const bs = document.getElementById('battStatus');
  const bcd = document.getElementById('battChargeDetail');

  // CHARGE: ⚡ verde pulsante si cargando, ⚡ gris atenuado si no
  const setCharge = (pulse) => {
    if (!bcd) return;
    if (pulse) {
      bcd.innerHTML = '<span class="fas fa-bolt-lightning animated-green charging-glow"></span>';
    } else {
      bcd.innerHTML = '<span class="fas fa-bolt-lightning charge-idle"></span>';
    }
  };

  if (bs) {
    if (SML.fullBatt) {
      setDataHTML(bs, '<span class="fas fa-check-circle animated-green"></span>');
      setCharge(false);
    } else if (SML.charging) {
      // Enchufado, cargando
      setDataHTML(bs, '<span class="fas fa-plug animated-green"></span>');
      setCharge(true);
    } else if (dispLevel <= 30) {
      // Batería baja sin corriente
      const cls = dispLevel <= 15 ? 'animated-red-fast' : 'animated-red';
      setDataHTML(bs, `<span class="fas fa-exclamation-triangle ${cls}"></span>`);
      setCharge(false);
    } else {
      // Funcionando a batería — icono según nivel
      let icon, cls;
      if (dispLevel <= 50) {
        icon = 'fa-battery-quarter';
        cls = 'animated-orange';
      } else if (dispLevel <= 75) {
        icon = 'fa-battery-half';
        cls = 'animated-yellow';
      } else {
        icon = 'fa-battery-three-quarters';
        cls = 'animated-green';
      }
      setDataHTML(bs, `<span class="fas ${icon} ${cls}"></span>`);
      setCharge(false);
    }
  }

  // ── LAMP / NEOPIXEL ──
  if (data.lampstatus !== undefined) {
    const lt = document.getElementById('lampToggle');
    if (lt) togglePowerCard(lt, data.lampstatus === 'on');
  }
  if (data.neostatus !== undefined) {
    SML.powerOn = data.neostatus === 'on';
    const neo = document.getElementById('neoToggle');
    if (neo) togglePowerCard(neo, SML.powerOn);
    updatePeekEffectInfo();
    // Si apagan el NeoPixel remotamente, detener random
    if (!SML.powerOn) {
      if (SML.randomFXMode) stopRandomFX();
      if (SML.randomVUMode) stopRandomVU();
    }
  }

  // ── BRIGHTNESS (FAB pill slider) ──
  if (data.neobrightness !== undefined) {
    // Ignorar broadcasts stale si el usuario acaba de cambiar brillo
    if (SML._lastBrightnessSent && Date.now() - SML._lastBrightnessSent < 400) {
      // skip — nuestro propio eco o broadcast anterior
    } else {
      SML.brightness = data.neobrightness;
      if (typeof SML._initFabBrightness === 'function') {
        SML._initFabBrightness(data.neobrightness);
      }
    }
  }

  // ── COLOR ──
  if (data.color) {
    // Ignorar broadcasts stale si el usuario acaba de cambiar color
    // (evita que un broadcast de una acción anterior salte el puntero
    //  cuando haces clics rápidos en el picker)
    if (!SML._lastColorSent || Date.now() - SML._lastColorSent >= 400) {
      SML.r = data.color.r ?? SML.r;
      SML.g = data.color.g ?? SML.g;
      SML.b = data.color.b ?? SML.b;
      updateSolidIcon(SML.r, SML.g, SML.b);
      if (SML.colorPicker && !SML._colorDragging) {
        SML._colorRemoteLock = true;
        SML.colorPicker.color.set({ r: SML.r, g: SML.g, b: SML.b });
        SML._colorRemoteLock = false;
      }
    }
  }

  // ── RANDOM MODE (check FIRST — overrides individual effect cards on ALL clients) ──
  //   El backend incluye randomMode en cada notifyClients. 0=off, 1=randomFX, 2=randomVU.
  //   notifySensorData NO incluye randomMode — no altera las flags.
  if (data.randomMode !== undefined) {
    if (data.randomMode === 1) {
      SML.randomFXMode = true;
      SML.randomVUMode = false;
      SML.effectId = 99;
      $$('.effect-card').forEach(c =>
        c.classList.toggle('active', parseInt(c.dataset.effectId) === 99)
      );
    } else if (data.randomMode === 2) {
      SML.randomVUMode = true;
      SML.randomFXMode = false;
      SML.effectId = 100;
      $$('.effect-card').forEach(c =>
        c.classList.toggle('active', parseInt(c.dataset.effectId) === 100)
      );
    } else {
      // randomMode === 0 — limpiar flags
      SML.randomFXMode = false;
      SML.randomVUMode = false;
    }
  }

  // ── EFFECT ──
  // Find which effect is "on" — only if NOT in random mode (already handled above)
  let fxFound = false;

  // Regular effect lookup — skip if random mode is active
  if (data.randomMode === undefined || data.randomMode === 0) {
    // Map effect JSON names (from EffectRegistry.cpp) to web effect IDs
    const effectNameToId = {
      'fireStatus': 1,
      'movingdotStatus': 2,
      'rainbowbeatStatus': 3,
      'rwbStatus': 4,
      'rippleStatus': 5,
      'ballsStatus': 6,
      'juggleStatus': 7,
      'sinelonStatus': 8,
      'cometStatus': 9,
      'breathStatus': 10,
      'colorSweepStatus': 11,
      'rainbowVUStatus': 12,
      'oldVUStatus': 13,
      'rainbowHueVUStatus': 14,
      'rippleVUStatus': 15,
      'threebarsVUStatus': 16,
      'oceanVUStatus': 17,
      'tempNEOStatus': 18,
      'battNEOStatus': 19,
      'colorWipeStatus': 20,
      'theaterChaseStatus': 21,
      'runningLightsStatus': 22,
      'dissolveStatus': 23,
      'dualScanStatus': 24,
      'fadeStatus': 25,
      'meteorStatus': 26,
      'sparkleStatus': 27,
      'bpmStatus': 28,
      'plasmaStatus': 29,
      'fireworksStatus': 30,
      'lightningStatus': 31,
      'pride2015Status': 32,
      'colorwavesStatus': 33,
      'pacificaStatus': 34,
      'twinkleFOXStatus': 35,
      'auroraStatus': 36,
      'popcornStatus': 37,
      'larsonScannerStatus': 38,
      'heartbeatStatus': 39,
      'icuStatus': 40,
      'sunriseStatus': 41,
      'dripStatus': 42,
      'candleStatus': 43,
      'chunchunStatus': 44,
      'halloweenEyesStatus': 45,
      'gravimeterVUStatus': 46,
      'noisemeterVUStatus': 47,
      'djlightVUStatus': 48,
      'ps1dgeqVUStatus': 49,
      'paletteBlendVUStatus': 50,
    };

    for (const [key, id] of Object.entries(effectNameToId)) {
      if (data[key] === 'on') {
        if (SML.effectId !== id) {
          SML.effectId = id;
          $$('.effect-card').forEach(c => {
            c.classList.toggle('active', parseInt(c.dataset.effectId) === id);
          });
        }
        fxFound = true;
        break;
      }
    }
    // Also check for effect statuses NOT in the map (IDs 38+)
    if (!fxFound) {
      for (const key of Object.keys(data)) {
        if (key.endsWith('Status') && data[key] === 'on') {
          fxFound = true;
          break;
        }
      }
    }
    // If NO effect is "on" but Neo IS on → Solid (not in EffectRegistry, never has a Status:"on")
    // ⚠ Only trigger if this message actually contains effect/status data
    //   (notifySensorData sends lightweight payloads WITHOUT Status fields)
    if (!fxFound && SML.powerOn) {
      const hasEffectData = Object.keys(data).some(k => k.endsWith('Status') || k === 'neostatus');
      if (hasEffectData) {
        SML.effectId = 0;
        $$('.effect-card').forEach(c => {
          c.classList.toggle('active', parseInt(c.dataset.effectId) === 0);
        });
        updateSolidIcon(SML.r, SML.g, SML.b);
      }
    }

    // Direct effectId from server
    if (data.effectId !== undefined) {
      SML.effectId = data.effectId;
      $$('.effect-card').forEach(c => {
        c.classList.toggle('active', parseInt(c.dataset.effectId) === data.effectId);
      });
    }
  }

  // ── PALETTE (top-level, enviado en cada broadcast) ──
  if (data.palette !== undefined) {
    const effId = data.effectId !== undefined ? data.effectId : SML.effectId;
    if (!liveEffectParams[effId]) liveEffectParams[effId] = {};
    liveEffectParams[effId].palette = parseInt(data.palette);
    // Update palette selector UI if open
    const containers = [
      document.getElementById('effectOffcanvasBody'),
      document.getElementById('paramSheetBody')
    ].filter(Boolean);
    containers.forEach(container => {
      container.querySelectorAll('.palette-swatch').forEach(s => {
        const isSelected = parseInt(s.dataset.paletteIndex) === parseInt(data.palette);
        s.classList.toggle('selected', isSelected);
        s.querySelector('.palette-check-mark').textContent = isSelected ? '✓' : '';
      });
    });
  }

  // Sincronizar botón de configuración en Peek
  updatePeekEffectInfo();

  // ── EFFECT PARAMS (real-time from server) ──
  // ESP32 sends { effectId: N, params: { speed: 120, intensity: 50, ... } }
  if (data.params && typeof data.params === 'object') {
    // Guardar en cache de valores activos para que renderEffectParams()
    // use los valores reales del ESP32, no los defaults del metadata.
    const paramsEffId = data.effectId !== undefined ? data.effectId : SML.effectId;
    if (!liveEffectParams[paramsEffId]) liveEffectParams[paramsEffId] = {};
    Object.assign(liveEffectParams[paramsEffId], data.params);

    const offcanvasBody = document.getElementById('effectOffcanvasBody');
    const sheetBody = document.getElementById('paramSheetBody');
    const containers = [offcanvasBody, sheetBody].filter(Boolean);
    containers.forEach(container => {
      Object.entries(data.params).forEach(([key, val]) => {
        const input = container.querySelector(`input[data-key="${key}"]`);
        if (!input) return;
        if (input.type === 'checkbox') {
          const shouldCheck = val === true || val === 1 || val === '1';
          if (input.checked !== shouldCheck) {
            console.debug(`[DEBUG] Sync check1: server sent ${JSON.stringify(val)}, shouldCheck=${shouldCheck}, was=${input.checked} → setting to ${shouldCheck}`);
            input.checked = shouldCheck;
          }
        } else {
          const numVal = parseInt(val);
          if (parseInt(input.value) !== numVal) {
            input.value = numVal;
            const valSpan = input.nextElementSibling;
            if (valSpan) valSpan.textContent = numVal;
          }
        }
      });
      // Update palette selection
      if (data.params.palette !== undefined) {
        const palIdx = parseInt(data.params.palette);
        container.querySelectorAll('.palette-swatch').forEach(s => {
          const isSelected = parseInt(s.dataset.paletteIndex) === palIdx;
          s.classList.toggle('selected', isSelected);
          s.querySelector('.palette-check-mark').textContent = isSelected ? '✓' : '';
        });
      }
    });
  }

  // ── EFFECT METADATA (del efecto activo, refresco continuo) ──
  // Solo cacheamos — NO re-renderizar (eso resetea sliders al default).
  // La UI se actualiza via data.params arriba y via HTTP /fxdata al iniciar.
  if (data.meta && typeof data.meta === 'string' && SML.effectId > 0) {
    const parsed = parseEffectMeta(data.meta);
    if (parsed) {
      if (!effectMetaCache[SML.effectId]) effectMetaCache[SML.effectId] = parsed;
    }
  }

  // ── BLUETOOTH ──
  if (data.btstatus !== undefined) {
    SML.btPower = data.btstatus === 'on';
    // Bridge to player.js sync (star-tab UI update)
    if (typeof window.playerSync === 'function') {
      window.playerSync({ bt_powerState: SML.btPower });
    }
  }

  // ── BATTERY EFFECT STATUS ──
  if (data.battNEOStatus !== undefined) {
    SML.battEffectActive = data.battNEOStatus === 'on';
    const bToggle = document.getElementById('batteryToggle');
    if (bToggle) bToggle.classList.toggle('active', SML.battEffectActive);
  }

  // ── TEMPERATURE EFFECT STATUS ──
  if (data.tempNEOStatus !== undefined) {
    SML.tempEffectActive = data.tempNEOStatus === 'on';
    const tToggle = document.getElementById('tempToggle');
    if (tToggle) tToggle.classList.toggle('active', SML.tempEffectActive);
  }

  // ── WIFI / NETWORK (state + status bar) ──
  if (data.rssi !== undefined) {
    SML.wifiRSSI = data.rssi;
    updateWiFiBars(data.rssi);
  }

  if (data.ip !== undefined) {
    SML.deviceIP = data.ip;
  }

  // ── WEBSOCKET CLIENT COUNT (status bar eye icon, slaves only) ──
  if (data.wsSlaves !== undefined && data.wsMax !== undefined) {
    updateWSClientCount(data.wsSlaves, data.wsMax);
  }

  // ── WEBSOCKET CLIENT LIST (Config tab) ──
  if (data.wsClientList !== undefined && Array.isArray(data.wsClientList)) {
    if (typeof updateWSClientList === 'function') {
      updateWSClientList(data.wsClientList, data.wsActionLog);
    }
  }
  // ── WEBSOCKET ACTION LOG (mensaje independiente, sin client list) ──
  else if (data.wsActionLog !== undefined && Array.isArray(data.wsActionLog)) {
    if (typeof updateWSClientList === 'function') {
      updateWSClientList(null, data.wsActionLog);
    }
  }

  // ── CONFIG TAB (delegado a config.js) ──
  if (typeof updateSystemInfo === 'function') {
    updateSystemInfo(data);
  }

  // ── WEATHER HEAT INDEX ──
  if (data.temperature !== undefined && data.humidity !== undefined) {
    const hi = document.getElementById('weatherHeatIndex');
    if (hi) {
      // Simple heat index approximation
      const T = data.temperature;
      const H = data.humidity;
      let feels = T;
      if (T >= 27) {
        feels = -8.784695 + 1.61139411 * T + 2.338548 * H
               - 0.14611605 * T * H - 0.012308094 * T * T
               - 0.016424828 * H * H + 0.002211732 * T * T * H
               + 0.00072546 * T * H * H - 0.000003582 * T * T * H * H;
      }
      setDataValue(hi, feels.toFixed(1));
    }
  }
}

function updateConnectionStatus(connected) {
  const indicator = document.getElementById('connectionIndicator');
  if (indicator) {
    indicator.textContent = connected ? '●' : '○';
    indicator.style.color = connected
      ? 'var(--accent-success)'
      : 'var(--accent-danger)';
  }
}

function updateWSClientCount(slaves, max) {
  const indicator = document.getElementById('wsIndicator');
  const icon = document.getElementById('wsEyeIcon');
  const countEl = document.getElementById('wsCount');
  if (!indicator || !icon) return;

  if (slaves === 0) {
    // Only master or nobody — hide the eye icon entirely
    indicator.style.display = 'none';
    return;
  }

  // Show with slave count
  indicator.style.display = 'flex';
  if (countEl) countEl.textContent = slaves;

  // Color by load: green (few) → yellow (moderate) → red pulsing (near limit)
  // max = 8 total clients, so slaves max = 7
  icon.classList.remove('ws-warn', 'ws-danger');
  if (slaves >= max - 1) {
    icon.classList.add('ws-danger');    // 7 slaves = all slots full
  } else if (slaves >= max - 3) {
    icon.classList.add('ws-warn');      // 5-6 slaves = approaching limit
  }
  // else green (1-4 slaves, plenty of room)
}

// ============================================================================
// PLAYER CONTROLS (bridge for player.js)
// ============================================================================

function initPlayerBridge() {
  // player.js (IIFE) auto-initializes — no bridge action needed here.
}

// sendWebSocketCommand is already declared in player.js — bridge not needed here.

// ============================================================================
// TOAST — FIFO queue, one at a time
// ============================================================================

const _toastQueue = [];
let _toastActive = false;

function showToast(message, type) {
  type = type || 'info';
  _toastQueue.push({ message, type });
  _processToastQueue();
}

function _processToastQueue() {
  if (_toastActive || _toastQueue.length === 0) return;
  _toastActive = true;

  const { message, type } = _toastQueue.shift();
  const container = document.getElementById('toastContainer');
  if (!container) {
    _toastActive = false;
    return;
  }

  const toast = document.createElement('div');
  toast.className = 'toast';
  toast.classList.add(type);

  // Icon map
  const icons = {
    success: '<span class="fas fa-check-circle" style="color:var(--accent-success)"></span>',
    error:   '<span class="fas fa-exclamation-circle" style="color:var(--accent-danger)"></span>',
    warning: '<span class="fas fa-exclamation-triangle" style="color:var(--accent-warning)"></span>',
    info:    '<span class="fas fa-info-circle" style="color:var(--accent-secondary)"></span>',
  };

  toast.innerHTML = `
    <span class="toast-icon">${icons[type] || icons.info}</span>
    <span class="toast-text">${message}</span>
    <span class="toast-progress"></span>
  `;

  container.appendChild(toast);

  // Show next after this one finishes (animation duration + visible time)
  setTimeout(() => {
    if (toast.parentNode) toast.remove();
    _toastActive = false;
    _processToastQueue();
  }, 3000);
}

// ============================================================================
// RESIZE
// ============================================================================

function handleResize() {
  SML.isDesktop = window.innerWidth >= 768;
  if (SML.isDesktop) {
    // Close bottom sheet when resizing to desktop
    const sheet = document.getElementById('paramBottomSheet');
    const modOv = document.getElementById('paramModalOverlay');
    if (sheet) sheet.classList.remove('open');
    if (modOv) modOv.classList.remove('open');
  }
}

// ============================================================================
// WIFI / LED SAVE
// ============================================================================

async function saveWiFiConfig() {
  const ssid = document.getElementById('wifiSsid')?.value.trim();
  const pass = document.getElementById('wifiPass')?.value;
  if (!ssid) { showToast('Please enter an SSID', 'warning'); return; }
  if (!confirm(`Change WiFi to "${ssid}"? The device will reconnect and you may lose connection.`)) return;

  try {
    const params = new URLSearchParams();
    params.append('ssid', ssid);
    params.append('password', pass || '');

    const resp = await fetch('/save-wifi', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: params.toString()
    });
    const data = await resp.json();
    if (data.status === 'success') {
      showToast('WiFi saved. Reconnecting...', 'success');
    } else {
      showToast('Error: ' + (data.message || 'unknown'), 'error');
    }
  } catch (e) {
    showToast('Connection lost — device is restarting', 'error');
  }
}

// ============================================================================
// SKELETON HELPER
// ============================================================================

/** Sets textContent on an element and removes its skeleton loading state. */
function setDataValue(el, value, suffix) {
  if (!el) return;
  el.textContent = (value !== undefined && value !== null) ? (value + (suffix || '')) : '--';
  el.classList.remove('skeleton');
}

/** Sets innerHTML on an element and removes its skeleton loading state. */
function setDataHTML(el, html) {
  if (!el) return;
  el.innerHTML = html || '--';
  el.classList.remove('skeleton');
}

// ============================================================================
// INIT
// ============================================================================

document.addEventListener('DOMContentLoaded', () => {
  // Theme
  initTheme();

  // Add skeleton loading to all data-bearing elements
  document.querySelectorAll('.stat-value, .info-value, #weatherHumVal, #weatherHeatIndex, #sysUptime, #sysHeap, #sysRSSI, #sysVersion, #deviceIP, #sysLEDs, #battPercentDetail, #battVoltageDetail, #battStatus, #battChargeDetail')
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

  // Peek canvas config gear — abre el panel del efecto actual
  document.getElementById('peekCanvasConfigBtn')?.addEventListener('click', () => {
    const activeCard = document.querySelector('.effect-card.active');
    if (!activeCard) return;
    const effId = parseInt(activeCard.dataset.effectId);
    showEffectConfig(effId, activeCard);
  });

  // Theme options
  $$('.theme-option').forEach(opt => {
    opt.addEventListener('click', () => setTheme(opt.dataset.theme));
  });

  // Config save buttons
  const wifiBtn = document.getElementById('wifiSaveBtn');
  if (wifiBtn) wifiBtn.addEventListener('click', saveWiFiConfig);

  // Enter key on WiFi pass
  const wifiPass = document.getElementById('wifiPass');
  if (wifiPass) {
    wifiPass.addEventListener('keydown', (e) => {
      if (e.key === 'Enter') saveWiFiConfig();
    });
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
