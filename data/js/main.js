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

  // ── Brightness slider standalone ──────────────────────────────────────
  const brightSlider = document.getElementById('brightnessSlider');
  const brightVal = document.getElementById('brightnessValue');
  if (brightSlider && brightVal) {
    brightSlider.addEventListener('input', () => {
      const v = parseInt(brightSlider.value);
      SML.brightness = v;
      brightVal.textContent = v;
      // Update gradient fill on slider track
      const pct = (v / brightSlider.max) * 100;
      brightSlider.style.setProperty('--slider-pct', pct + '%');
      // Sync peek brightness slider
      const peekBright = document.getElementById('peekBrightness');
      const peekBrightVal = document.getElementById('peekBrightnessVal');
      if (peekBright) peekBright.value = v;
      if (peekBrightVal) peekBrightVal.textContent = v;
    });
    brightSlider.addEventListener('change', () => {
      SML._lastBrightnessSent = Date.now();
      sendCmd({ action: 'slider', brightness: SML.brightness });
    });
    SML._brightnessSlider = brightSlider;
    // Init gradient fill position
    const initPct = (parseInt(brightSlider.value) / brightSlider.max) * 100;
    brightSlider.style.setProperty('--slider-pct', initPct + '%');
  }

  // ── Peek brightness slider (synced with main brightness) ──
  const peekBright = document.getElementById('peekBrightness');
  const peekBrightVal = document.getElementById('peekBrightnessVal');
  if (peekBright && peekBrightVal) {
    peekBright.addEventListener('input', () => {
      const v = parseInt(peekBright.value);
      peekBrightVal.textContent = v;
      SML.brightness = v;
      // Sync main slider
      if (SML._brightnessSlider) {
        SML._brightnessSlider.value = v;
        const pct = (v / SML._brightnessSlider.max) * 100;
        SML._brightnessSlider.style.setProperty('--slider-pct', pct + '%');
      }
      const bv = document.getElementById('brightnessValue');
      if (bv) bv.textContent = v;
    });
    peekBright.addEventListener('change', () => {
      SML._lastBrightnessSent = Date.now();
      sendCmd({ action: 'slider', brightness: SML.brightness });
    });
  }

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

      const wasActive = card.classList.contains('active');

      // Si cambia a otro efecto, cerrar config abierta
      if (activeCard && activeCard !== card) {
        closeEffectConfig();
      }

      // Enviar comando de efecto siempre
      SML.effectId = effId;
      sendCmd({ effectId: effId });

      // Actualizar estado visual
      cards.forEach(c => c.classList.remove('active'));
      card.classList.add('active');
      activeCard = card;
      // Solid icon: color del picker solo si está activo, gris si no
      updateSolidIcon(SML.r, SML.g, SML.b);

      // Sincronizar botón de configuración en la tarjeta Peek
      updatePeekEffectInfo();

      // Segundo click en la misma card → mostrar config (si tiene parámetros)
      if (wasActive) {
        const meta = effectMetaCache[effId];
        const hasParams = meta && meta.params && meta.params.length > 0;
        if (hasParams) showEffectConfig(effId, card);
      }
    });
  });
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
  const config = effectMetaCache[effId];
  if (!config || !config.params || config.params.length === 0) {
    container.innerHTML = '';
    return;
  }

  // Usar valores activos del ESP32 si existen, sino los defaults del metadata
  const live = liveEffectParams[effId] || {};

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
  }).join('') + `
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
        sendCmd(msg);
      }
    });
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

  return { name, params };
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
  }

  // ── BRIGHTNESS (slider standalone) ──
  if (data.neobrightness !== undefined) {
    // Ignorar broadcasts stale si el usuario acaba de cambiar brillo
    if (SML._lastBrightnessSent && Date.now() - SML._lastBrightnessSent < 400) {
      // skip — nuestro propio eco o broadcast anterior
    } else {
      SML.brightness = data.neobrightness;
      if (SML._brightnessSlider) {
        SML._brightnessSlider.value = data.neobrightness;
        const pct = (data.neobrightness / SML._brightnessSlider.max) * 100;
        SML._brightnessSlider.style.setProperty('--slider-pct', pct + '%');
        const val = document.getElementById('brightnessValue');
        if (val) val.textContent = data.neobrightness;
      }
      // Sync peek brightness slider
      const peekBright = document.getElementById('peekBrightness');
      const peekBrightVal = document.getElementById('peekBrightnessVal');
      if (peekBright) peekBright.value = data.neobrightness;
      if (peekBrightVal) peekBrightVal.textContent = data.neobrightness;
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
    'colorWipeStatus': 21,
    'theaterChaseStatus': 22,
    'runningLightsStatus': 23,
    'dissolveStatus': 24,
    'dualScanStatus': 25,
    'fadeStatus': 26,
    'meteorStatus': 27,
    'sparkleStatus': 28,
    'fire2012Status': 29,
    'bpmStatus': 30,
    'plasmaStatus': 31,
    'fireworksStatus': 32,
    'lightningStatus': 33,
    'pride2015Status': 34,
    'colorwavesStatus': 35,
    'pacificaStatus': 36,
    'twinkleFOXStatus': 37,
    'auroraStatus': 38,
    'popcornStatus': 39,
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

function showToast(message, type) {
  const container = document.getElementById('toastContainer');
  if (!container) return;
  const toast = document.createElement('div');
  toast.className = 'toast';

  // Type (success, error, warning, info — defaults to info)
  type = type || 'info';
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
      sendCmd({ action: 'toggleBatt' });
    });
  }

  // Temperature toggle (tap thermometer to toggle temp LED effect)
  const tempToggle = document.getElementById('tempToggle');
  if (tempToggle) {
    tempToggle.addEventListener('click', () => {
      if (!SML.powerOn) { showToast('Turn on the NeoPixel strip first', 'warning'); return; }
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


  // Resize
  window.addEventListener('resize', handleResize);

  // Load effect metadata from server (caches into effectMetaCache)
  fetchFxdata();

  // Connect WebSocket
  connectWS();

  // Activate initial tab
  switchTab(SML.currentTab);

  // Initial sync for Peek config button state
  updatePeekEffectInfo();

  // Notify preserved modules are ready
  if (typeof initBatteryAnimation === 'function') initBatteryAnimation();
});
