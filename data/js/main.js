/* ──────────────────────────────────────────────────────────────────────────────
   main.js — SML Web UI (Phase 0 - protocol compatible)
   WebSocket handler + Tab routing + Theme system + UI controllers
   ──────────────────────────────────────────────────────────────────────────────
   ⚠ CRITICAL: No snprintf() anywhere — ESP32 crashes with format specifiers.
   All values sent as raw JSON numbers, never pre-formatted strings.

   Protocol (ESP32 → Client):
     {bars, battVoltage, level, charging, fullbatt,
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
  batteryV: 0,
  charging: false,
  fullBatt: false,
  btPower: false,
  wifiRSSI: -70,
  deviceIP: '0.0.0.0',
  uptime: 0,

  // UI
  currentTab: 'tabLamp',
  isDesktop: window.innerWidth >= 768,
  theme: localStorage.getItem('sml-theme') || 'sml-classic',

  // Volume (15 steps, 0-14, middle = 7)
  volumeLevel: 7,
  playing: false,
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

  // Stop peek render when leaving peek tab
  if (tabId !== 'tabPeek' && typeof peek !== 'undefined' && peek) {
    peek.stop();
    const peekToggle = document.getElementById('peekToggle');
    if (peekToggle) {
      peekToggle.textContent = '▶ Live';
      peekToggle.classList.remove('active');
    }
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

  fill.className = 'batt-fill';
  if (clamped > 60) fill.classList.add('high');
  else if (clamped > 20) fill.classList.add('medium');
  else fill.classList.add('low');

  iconContainer?.classList.toggle('batt-charging', charging);
  iconContainer?.classList.toggle('charging', charging);
  if (chargingEl) chargingEl.style.display = charging ? 'inline' : 'none';
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
  document.documentElement.setAttribute('data-theme', themeName);
  $$('.theme-option').forEach(opt => {
    opt.classList.toggle('active', opt.dataset.theme === themeName);
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
    });
  }

  // Color picker (iro.js) — value slider = brightness control
  if (typeof iro !== 'undefined' && document.getElementById('colorPicker')) {
    const colorPicker = new iro.ColorPicker('#colorPicker', {
      width: 220,
      color: { h: 0, s: 0, v: 51 },  // V=51% = brightness 130
      borderWidth: 0,
      handleRadius: 8,
      layout: [
        { component: iro.ui.Wheel },
        { component: iro.ui.Slider, options: { sliderType: 'value' } },
      ],
    });

    let isRemoteUpdate = false;
    let lastBrightness = -1; // force first send
    colorPicker.on('color:change', (color) => {
      if (isRemoteUpdate) return;
      SML.r = color.rgb.r;
      SML.g = color.rgb.g;
      SML.b = color.rgb.b;
      // Map HSV value (0-100) to brightness (0-255)
      SML.brightness = Math.round(color.hsv.v * 2.55);
    });

    colorPicker.on('input:end', () => {
      sendCmd({ action: 'slider', brightness: SML.brightness });
      sendCmd({ action: 'picker', color: { r: SML.r, g: SML.g, b: SML.b } });
    });

    SML.colorPicker = colorPicker;
  }

  // Effect cards
  initEffectCards();
}


function initEffectCards() {
  const cards = $$('.effect-card');
  let activeCard = null;

  cards.forEach(card => {
    card.addEventListener('click', () => {
      const effId = parseInt(card.dataset.effectId);
      if (isNaN(effId)) return;

      // Update active state
      cards.forEach(c => c.classList.remove('active'));
      card.classList.add('active');
      activeCard = card;

      SML.effectId = effId;

      // Send effect command to ESP32
      sendCmd({ effectId: effId });

      // Show params panel only if effect has configurable params
      const hasParams = EFFECT_PARAMS[effId] && EFFECT_PARAMS[effId].params && EFFECT_PARAMS[effId].params.length > 0;
      if (hasParams) showEffectConfig(effId, card);
    });
  });
}

function showEffectConfig(effId, cardEl) {
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

function renderEffectParams(effId, container) {
  const config = EFFECT_PARAMS[effId];
  if (!config || !config.params || config.params.length === 0) {
    container.innerHTML = '';
    return;
  }

  container.innerHTML = config.params.map(p => `
    <div class="param-row">
      <label>${p.label}</label>
      <input type="range" min="${p.min}" max="${p.max}" value="${p.default}"
             data-key="${p.key}">
      <span class="param-value">${p.default}</span>
    </div>
  `).join('');

  container.querySelectorAll('input[type="range"]').forEach(input => {
    const valSpan = input.nextElementSibling;
    input.addEventListener('input', () => { valSpan.textContent = input.value; });
    input.addEventListener('change', () => {
      const msg = { action: 'setParams', effectId: effId };
      msg[input.dataset.key] = parseInt(input.value);
      sendCmd(msg);
    });
  });
}

// ============================================================================
// EFFECT PARAMETERS DEFINITIONS
// ============================================================================

const EFFECT_PARAMS = {
  // NOTE: keys = HTML effectId (matches ESP32 effectId). Params list the actual
  // setters each Effect subclass uses (from EffectRegistry order).
  1: { name: 'Fire', params: [
    { key: 'custom1', label: 'Cooling', type: 'range', min: 0, max: 255, default: 55 },
    { key: 'custom2', label: 'Sparking', type: 'range', min: 0, max: 255, default: 50 },
  ]},
  5: { name: 'Ripple', params: [
    { key: 'custom1', label: 'Size', type: 'range', min: 1, max: 50, default: 3 },
  ]},
  6: { name: 'Twinkle', params: [
    { key: 'speed', label: 'Speed', type: 'range', min: 0, max: 255, default: 100 },
    { key: 'intensity', label: 'Density', type: 'range', min: 0, max: 255, default: 50 },
  ]},
  7: { name: 'Bouncing Balls', params: [
    { key: 'custom1', label: 'Balls', type: 'range', min: 1, max: 10, default: 3 },
    { key: 'custom2', label: 'Gravity', type: 'range', min: 1, max: 20, default: 10 },
  ]},
  8: { name: 'Juggle', params: [
    { key: 'speed', label: 'Speed', type: 'range', min: 0, max: 255, default: 100 },
    { key: 'intensity', label: 'Intensity', type: 'range', min: 0, max: 255, default: 128 },
    { key: 'custom1', label: 'Dots', type: 'range', min: 1, max: 8, default: 3 },
  ]},
  9: { name: 'Sinelon', params: [
    { key: 'speed', label: 'BPM', type: 'range', min: 0, max: 255, default: 23 },
    { key: 'custom1', label: 'Fade', type: 'range', min: 1, max: 50, default: 2 },
  ]},
  10: { name: 'Comet', params: [
    { key: 'speed', label: 'Speed', type: 'range', min: 0, max: 255, default: 80 },
    { key: 'custom1', label: 'Trail', type: 'range', min: 1, max: 50, default: 10 },
  ]},
};

// ============================================================================
// WEBSOCKET
// ============================================================================

function connectWS() {
  const protocol = location.protocol === 'https:' ? 'wss:' : 'ws:';
  const url = `${protocol}//${location.host}/ws`;

  if (SML.ws && SML.ws.readyState === WebSocket.OPEN) return;

  try {
    SML.ws = new WebSocket(url);
  } catch (e) {
    console.error('WS connection failed:', e);
    scheduleReconnect();
    return;
  }

  // Expose globals for player.js
  websocket = SML.ws;

  SML.ws.onopen = () => {
    SML.connected = true;
    updateConnectionStatus(true);
    clearTimeout(SML.wsReconnectTimer);
    // Remove skeletons when connected
    document.querySelectorAll('.skeleton').forEach(el => el.classList.remove('skeleton'));
  };

  SML.ws.onclose = () => {
    SML.connected = false;
    updateConnectionStatus(false);
    // Re-add skeleton to data elements when disconnected
    document.querySelectorAll('.stat-value, .info-value, #weatherTemp, #weatherHumVal, #weatherHeatIndex, #battVolt, #sysUptime, #sysHeap, #sysRSSI, #sysVersion, #deviceIP, #battPercentDetail, #battVoltageDetail, #battStatus, #battChargeDetail')
      .forEach(el => {
        if (el.textContent === '--' || el.textContent === '--.-' || el.textContent === '--.--V' || el.textContent === '--%') {
          el.classList.add('skeleton');
        }
      });
    scheduleReconnect();
  };

  SML.ws.onerror = () => {};

  SML.ws.onmessage = (event) => {
    // Binary data for Peek (Phase 3)
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
  SML.wsReconnectTimer = setTimeout(connectWS, 3000);
}

function sendCmd(obj) {
  if (SML.ws && SML.ws.readyState === WebSocket.OPEN) {
    SML.ws.send(JSON.stringify(obj));
  }
}

function handleMessage(data) {
  // ── TEMPERATURE / HUMIDITY ──
  if (data.temperature !== undefined) {
    SML.temp = data.temperature;

    // Update thermometer animation
    const tempEl = document.getElementById('temperature');
    if (tempEl) {
      const value = parseFloat(data.temperature);
      const minTemp = 0, maxTemp = 50;
      const pct = Math.min(100, Math.max(0, ((value - minTemp) / (maxTemp - minTemp)) * 100));
      tempEl.style.height = pct + '%';
      tempEl.dataset.value = value.toFixed(1) + '°C';
    }

    // Weather display
    const wt = document.getElementById('weatherTemp');
    const wtv = document.getElementById('weatherTempVal');
    if (wt) wt.textContent = value.toFixed(1);
    if (wtv) wtv.textContent = value.toFixed(1);
  }

  if (data.humidity !== undefined) {
    SML.hum = data.humidity;
    const wh = document.getElementById('weatherHum');
    const whv = document.getElementById('weatherHumVal');
    if (wh) wh.textContent = Math.round(data.humidity);
    if (whv) whv.textContent = Math.round(data.humidity);
  }

  // ── BATTERY ──
  if (data.level !== undefined) {
    SML.battery = data.level;
    batt.level = data.level;
    updateBatteryBar(data.level, data.charging || false);
  }
  if (data.battVoltage !== undefined) {
    SML.batteryV = data.battVoltage;
    const bv = document.getElementById('battVolt');
    const bvd = document.getElementById('battVoltageDetail');
    if (bv) bv.textContent = data.battVoltage.toFixed(2) + ' V';
    if (bvd) bvd.textContent = data.battVoltage.toFixed(2) + 'V';
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

  // Battery detail stats
  const bpd = document.getElementById('battPercentDetail');
  if (bpd) bpd.textContent = (SML.battery || 0) + '%';
  const bs = document.getElementById('battStatus');
  if (bs) {
    if (SML.charging) bs.textContent = 'Charging';
    else if (SML.fullBatt) bs.textContent = 'Full';
    else bs.textContent = 'In Use';
  }
  const bcd = document.getElementById('battChargeDetail');
  if (bcd) {
    if (SML.charging) bcd.innerHTML = '<span class="fas fa-bolt"></span> Charging';
    else if (SML.fullBatt) bcd.innerHTML = '<span class="fas fa-check"></span> Full';
    else bcd.textContent = 'Battery';
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
  }

  if (data.neobrightness !== undefined) {
    SML.brightness = data.neobrightness;
    // Sync iro.js value slider (brightness = V * 2.55)
    if (SML.colorPicker) {
      const hsv = SML.colorPicker.color.hsv;
      SML.colorPicker.color.set({ h: hsv.h, s: hsv.s, v: Math.round(data.neobrightness / 2.55) });
    }
  }

  // ── COLOR ──
  if (data.color) {
    SML.r = data.color.r || SML.r;
    SML.g = data.color.g || SML.g;
    SML.b = data.color.b || SML.b;
    if (SML.colorPicker) {
      SML.colorPicker.color.set({ r: SML.r, g: SML.g, b: SML.b });
    }
  }

  // ── EFFECT ──
  // Effect active state is determined by effectName fields in the JSON
  // Each effect has a field like "firebutton": "on"/"off"
  // Map effect names to IDs for active card highlighting
  // Map effect JSON names (from EffectRegistry.cpp) to web effect IDs
  const effectNameToId = {
    'fireStatus': 1,
    'movingdotStatus': 2,
    'rainbowbeatStatus': 3,
    'rwbStatus': 4,
    'rippleStatus': 5,
    'twinkleStatus': 6,
    'ballsStatus': 7,
    'juggleStatus': 8,
    'sinelonStatus': 9,
    'cometStatus': 10,
    'breathStatus': 11,
    'colorSweepStatus': 12,
    'rainbowVUStatus': 13,
    'oldVUStatus': 14,
    'rainbowHueVUStatus': 15,
    'rippleVUStatus': 16,
    'threebarsVUStatus': 17,
    'oceanVUStatus': 18,
    'tempNEOStatus': 19,
    'battNEOStatus': 20,
  };

  // Find which effect is "on"
  for (const [key, id] of Object.entries(effectNameToId)) {
    if (data[key] === 'on') {
      if (SML.effectId !== id) {
        SML.effectId = id;
        $$('.effect-card').forEach(c => {
          c.classList.toggle('active', parseInt(c.dataset.effectId) === id);
        });
      }
      break;
    }
  }

  // Direct effectId from server
  if (data.effectId !== undefined) {
    SML.effectId = data.effectId;
    $$('.effect-card').forEach(c => {
      c.classList.toggle('active', parseInt(c.dataset.effectId) === data.effectId);
    });
  }

  // ── EFFECT PARAMS (real-time from server) ──
  // ESP32 sends { effectId: N, params: { speed: 120, intensity: 50, ... } }
  if (data.params && typeof data.params === 'object') {
    const offcanvasBody = document.getElementById('effectOffcanvasBody');
    const sheetBody = document.getElementById('paramSheetBody');
    const containers = [offcanvasBody, sheetBody].filter(Boolean);
    containers.forEach(container => {
      Object.entries(data.params).forEach(([key, val]) => {
        const input = container.querySelector(`input[data-key="${key}"]`);
        if (input && parseInt(input.value) !== val) {
          input.value = val;
          const valSpan = input.nextElementSibling;
          if (valSpan) valSpan.textContent = val;
        }
      });
    });
  }

  // ── BLUETOOTH ──
  if (data.btstatus !== undefined) {
    SML.btPower = data.btstatus === 'on';
    // Bridge to player.js sync (star-tab UI update)
    if (typeof window.playerSync === 'function') {
      window.playerSync({ bt_powerState: SML.btPower });
    }
  }

  // ── WIFI / NETWORK ──
  if (data.rssi !== undefined) {
    SML.wifiRSSI = data.rssi;
    updateWiFiBars(data.rssi);

    const rssiEl = document.getElementById('sysRSSI');
    if (rssiEl) rssiEl.textContent = data.rssi + ' dBm';
  }

  if (data.ip !== undefined) {
    SML.deviceIP = data.ip;
    const ipEl = document.getElementById('deviceIP');
    const sysIp = document.getElementById('sysIP');
    if (ipEl) ipEl.textContent = data.ip;
    if (sysIp) sysIp.textContent = data.ip;
  }

  if (data.ssid !== undefined) {
    const ssidEl = document.getElementById('wifiSsid');
    if (ssidEl && !ssidEl.value) ssidEl.placeholder = data.ssid;
  }

  // ── SYSTEM INFO ──
  if (data.uptime !== undefined) {
    const el = document.getElementById('sysUptime');
    if (el) {
      const s = data.uptime;
      const d = Math.floor(s / 86400);
      const h = Math.floor((s % 86400) / 3600);
      const m = Math.floor((s % 3600) / 60);
      el.textContent = (d > 0 ? d + 'd ' : '') + (h > 0 || d > 0 ? h + 'h ' : '') + m + 'm';
    }
  }

  if (data.heap !== undefined) {
    const el = document.getElementById('sysHeap');
    if (el) el.textContent = data.heap + ' KB';
  }

  // ── LED COUNT (if server sends it) ──
  if (data.n !== undefined) {
    const el = document.getElementById('sysLEDs');
    if (el) el.textContent = data.n;
    const countEl = document.getElementById('ledCount');
    if (countEl && !countEl.dataset.userChanged) countEl.value = data.n;
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
      hi.textContent = feels.toFixed(1);
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

// ============================================================================
// PLAYER CONTROLS (bridge for player.js)
// ============================================================================

function initPlayerBridge() {
  // player.js (IIFE) auto-initializes — no bridge action needed here.
}

// sendWebSocketCommand is already declared in player.js — bridge not needed here.

// ============================================================================
// TOAST
// ============================================================================

function showToast(message) {
  const container = document.getElementById('toastContainer');
  if (!container) return;
  const toast = document.createElement('div');
  toast.className = 'toast';
  toast.textContent = message;
  container.appendChild(toast);
  setTimeout(() => { if (toast.parentNode) toast.remove(); }, 3000);
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

function saveWiFiConfig() {
  const ssid = document.getElementById('wifiSsid')?.value.trim();
  const pass = document.getElementById('wifiPass')?.value;
  if (!ssid) { showToast('Please enter an SSID'); return; }
  if (!confirm(`Change WiFi to "${ssid}"? The device will reconnect and you may lose connection.`)) return;
  sendCmd({ ssid, pass });
  showToast('WiFi saved. Reconnecting...');
}

function saveLEDConfig() {
  const count = parseInt(document.getElementById('ledCount')?.value);
  if (!count || count < 1 || count > 300) { showToast('LED count must be 1-300'); return; }
  const maxMA = parseInt(document.getElementById('ledMaxMA')?.value) || 500;
  if (!confirm(`Save LED config (${count} LEDs, ${maxMA}mA) and reboot?`)) return;
  sendCmd({ ledcfg: { n: count, ma: maxMA } });
  showToast('LED config saved. Rebooting...');
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

// ============================================================================
// INIT
// ============================================================================

document.addEventListener('DOMContentLoaded', () => {
  // Theme
  initTheme();

  // Add skeleton loading to all data-bearing elements
  document.querySelectorAll('.stat-value, .info-value, #weatherTemp, #weatherHumVal, #weatherHeatIndex, #battVolt, #sysUptime, #sysHeap, #sysRSSI, #sysVersion, #deviceIP, #battPercentDetail, #battVoltageDetail, #battStatus, #battChargeDetail')
    .forEach(el => el.classList.add('skeleton'));

  // Tabs
  initTabs();

  // Lamp controls
  initLampControls();

  // Volume + BT are now handled by player.js (IIFE auto-inits)
  // player.js handles: play/pause, skip, BT star-tab, volume ring drag

  // Offcanvas close
  const closeOffcanvas = () => {
    const oc = document.getElementById('effectOffcanvas');
    const ov = document.getElementById('offcanvasOverlay');
    if (oc) oc.classList.remove('open');
    if (ov) ov.classList.remove('open');
  };
  document.querySelectorAll('.offcanvas-close').forEach(el => {
    el.addEventListener('click', closeOffcanvas);
  });
  const overlay = document.getElementById('offcanvasOverlay');
  if (overlay) overlay.addEventListener('click', closeOffcanvas);

  // Bottom sheet close
  const closeParamSheet = () => {
    const sheet = document.getElementById('paramBottomSheet');
    const modOv = document.getElementById('paramModalOverlay');
    if (sheet) sheet.classList.remove('open');
    if (modOv) modOv.classList.remove('open');
  };
  document.querySelectorAll('.param-sheet-close').forEach(el => {
    el.addEventListener('click', closeParamSheet);
  });
  const modOv = document.getElementById('paramModalOverlay');
  if (modOv) modOv.addEventListener('click', closeParamSheet);

  // Theme options
  $$('.theme-option').forEach(opt => {
    opt.addEventListener('click', () => setTheme(opt.dataset.theme));
  });

  // Config save buttons
  const wifiBtn = document.getElementById('wifiSaveBtn');
  if (wifiBtn) wifiBtn.addEventListener('click', saveWiFiConfig);
  const ledBtn = document.getElementById('ledSaveBtn');
  if (ledBtn) ledBtn.addEventListener('click', saveLEDConfig);

  // Enter key on WiFi pass
  const wifiPass = document.getElementById('wifiPass');
  if (wifiPass) {
    wifiPass.addEventListener('keydown', (e) => {
      if (e.key === 'Enter') saveWiFiConfig();
    });
  }

  // LED count user-change marker
  const ledCount = document.getElementById('ledCount');
  if (ledCount) {
    ledCount.addEventListener('input', () => { ledCount.dataset.userChanged = 'true'; });
  }

  // Resize
  window.addEventListener('resize', handleResize);

  // Connect WebSocket
  connectWS();

  // Activate initial tab
  switchTab(SML.currentTab);

  // Notify preserved modules are ready
  if (typeof initBatteryAnimation === 'function') initBatteryAnimation();
});
