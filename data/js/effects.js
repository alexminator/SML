/* ──────────────────────────────────────────────────────────────────────────────
   effects.js — SML Effect cards, favorites, random mode, config panels,
                playlist picker, effect params, palettes, metadata system
   Depends on: SML{}, sendCmd (websocket.js), showToast (ui.js),
               updateSolidIcon (controls.js), renderBatteryChart (battery-chart.js)
   ────────────────────────────────────────────────────────────────────────────── */

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
// EFFECT CARDS
// ============================================================================

function initEffectCards() {
  const container = document.querySelector('.effects-scroll-container');
  if (!container) return;
  let activeEffId = null;

  // ── Shared effect activation logic (used by both normal & VU handlers) ──
  function activateEffect(effId, wasActive, card) {
    // Random FX (ID 99)
    if (effId === 99) {
      handleRandomFXClick(card, wasActive);
      return;
    }
    // Random VU (ID 100)
    if (effId === 100) {
      handleRandomVUClick(card, wasActive);
      return;
    }

    // Normal effect cards — show config on re-click
    if (wasActive && !SML.randomFXMode && !SML.randomVUMode) {
      if (!SML.isMaster) {
        showToast('Only the master can configure effect parameters', 'info');
        return;
      }
      closeEffectConfig();
      showEffectConfig(effId, card);
      return;
    }

    // Stop random modes if active
    if (SML.randomFXMode) {
      stopRandomFX();
      sendCmd({ action: 'randomFX', state: false, effectId: effId });
    } else if (SML.randomVUMode) {
      stopRandomVU();
      sendCmd({ action: 'randomVU', state: false, effectId: effId });
    } else {
      sendCmd({ effectId: effId });
    }

    // Close config panel if switching effects
    if (activeEffId !== null && activeEffId !== effId) {
      closeEffectConfig();
    }

    // Update visual state on ALL effect-card instances
    SML.effectId = effId;
    $$('.effect-card').forEach(c => c.classList.remove('active'));
    $$('.effect-card').forEach(c => {
      if (parseInt(c.dataset.effectId) === effId) {
        c.classList.add('active');
      }
    });
    activeEffId = effId;
    updateSolidIcon(SML.r, SML.g, SML.b);
    updatePeekEffectInfo();

    // Second click → show config
    if (wasActive) {
      showEffectConfig(effId, card);
    }
  }

  // ── Handler #1: Normal effects (Effects tab inside .effects-scroll-container) ──
  container.addEventListener('click', e => {
    // Star click — toggle favorite
    const star = e.target.closest('.fav-star');
    if (star) {
      const effId = parseInt(star.dataset.effectId);
      if (!isNaN(effId)) toggleFavorite(effId);
      return;
    }

    // Card click
    const card = e.target.closest('.effect-card');
    if (!card) return;
    const effId = parseInt(card.dataset.effectId);
    if (isNaN(effId)) return;

    // If Random FX config is open and Playlist mode → add/remove from playlist
    if (effId !== 99 && effId !== 100) {
      const cfgOpen = document.querySelector('#effectOffcanvas.open, #paramBottomSheet.open');
      const rndTitle = cfgOpen && document.querySelector('#effectOffcanvasTitle, #paramSheetTitle');
      if (rndTitle && rndTitle.textContent === 'Random FX' &&
          localStorage.getItem('sml-random-mode') === 'playlist') {
        addToPlaylist(effId);
        return;
      }
    }

    // Toast warning if NeoPixel is off
    if (!SML.powerOn) {
      showToast('Turn on the NeoPixel strip first', 'warning');
      return;
    }

    const wasActive = document.querySelector(`.effect-card[data-effect-id="${effId}"].active`) !== null;
    activateEffect(effId, wasActive, card);
  });

  // ── Handler #2: VU effect cards (Music tab inside .effects-grid) ──
  const vuGrid = document.querySelector('#tabMusic .effects-grid');
  if (vuGrid) {
    vuGrid.addEventListener('click', e => {
      const card = e.target.closest('.effect-card');
      if (!card) return;
      const effId = parseInt(card.dataset.effectId);
      if (isNaN(effId)) return;

      // Toast warning if NeoPixel is off
      if (!SML.powerOn) {
        showToast('Turn on the NeoPixel strip first', 'warning');
        return;
      }

      const wasActive = document.querySelector(`.effect-card[data-effect-id="${effId}"].active`) !== null;
      activateEffect(effId, wasActive, card);
    });
  }
}

// ============================================================================
// FAVORITES — localStorage persistence + fav row sync
// ============================================================================

const FAV_KEY = 'sml-favorites';

function getFavorites() {
  try {
    return JSON.parse(localStorage.getItem(FAV_KEY)) || [];
  } catch {
    return [];
  }
}

function saveFavorites(ids) {
  localStorage.setItem(FAV_KEY, JSON.stringify(ids));
}

function syncFavRow() {
  const favRow = document.getElementById('favScrollRow');
  const favCat = document.getElementById('catFavorites');
  if (!favRow || !favCat) return;

  const ids = getFavorites();
  favRow.innerHTML = '';

  ids.forEach(effId => {
    // Find original card in a non-favorites category
    const original = document.querySelector(
      `.effect-category:not(#catFavorites) .effect-card[data-effect-id="${effId}"]`
    );
    if (!original) return;
    const clone = original.cloneNode(true);
    favRow.appendChild(clone);
  });

  // Show/hide favorites category
  favCat.style.display = ids.length ? '' : 'none';
}

function syncPlaylistRow() {
  const plRow = document.getElementById('playlistScrollRow');
  const plCat = document.getElementById('catPlaylist');
  if (!plRow || !plCat) return;

  const pl = getRandomPlaylist();
  plRow.innerHTML = '';

  pl.forEach(effId => {
    const original = document.querySelector(
      `.effect-category:not(#catFavorites):not(#catPlaylist) .effect-card[data-effect-id="${effId}"]`
    );
    if (!original) return;
    const clone = original.cloneNode(true);
    plRow.appendChild(clone);
  });

  plCat.style.display = pl.length ? '' : 'none';
}

function applyFavStars() {
  const ids = getFavorites();
  document.querySelectorAll('.fav-star').forEach(star => {
    const effId = parseInt(star.dataset.effectId);
    if (isNaN(effId)) return;
    star.classList.toggle('active', ids.includes(effId));
  });
}

function toggleFavorite(effId) {
  const ids = getFavorites();
  const idx = ids.indexOf(effId);

  if (idx >= 0) {
    ids.splice(idx, 1);
  } else {
    ids.push(effId);
  }

  saveFavorites(ids);
  syncFavRow();
  applyFavStars();
}

function initFavorites() {
  syncFavRow();
  syncPlaylistRow();
  applyFavStars();
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
  12, 13, 14, 15, 16, 17,     // RainbowVU → OceanVU (no Gravimeter, PS1DGEQ, Palette Blend removed)
  47, 48,                      // Noisemeter, DJ Light
];

// ── Random FX categories (maps HTML category IDs to effect IDs) ──
const RANDOM_CATEGORIES = [
  { id: 'catFundamentals', name: 'Fundamentals', effectIds: [0, 25, 11, 20, 23, 33, 34] },
  { id: 'catMoving',       name: 'Moving',       effectIds: [2, 3, 22, 24, 26, 9] },
  { id: 'catDynamics',     name: 'Dynamics',     effectIds: [1, 27, 30, 31, 36, 37, 35, 29] },
  { id: 'catPatterns',     name: 'Patterns',     effectIds: [4, 21, 28, 32, 5, 7, 8, 38] },
  { id: 'catStates',       name: 'States',       effectIds: [6, 10, 39, 40, 41, 42, 43, 44, 45] },
];

function getRandomDuration() {
  const val = parseInt(localStorage.getItem('sml-random-duration') || '8');
  return Math.max(3000, val * 1000);
}

function getRandomVUDurationSeconds() {
  const stored = localStorage.getItem('sml-random-vu-duration');
  const legacy = localStorage.getItem('sml-random-duration');
  const value = parseInt(stored || legacy || '8', 10);
  return Number.isFinite(value) ? Math.min(30, Math.max(3, value)) : 8;
}

function getRandomCategories() {
  try {
    const val = JSON.parse(localStorage.getItem('sml-random-categories'));
    if (Array.isArray(val)) return val;
  } catch {}
  // Default: all categories selected
  return RANDOM_CATEGORIES.map(c => c.id);
}

function getRandomPlaylist() {
  try {
    const p = JSON.parse(localStorage.getItem('sml-random-playlist'));
    return Array.isArray(p) ? p : [];
  } catch { return []; }
}

function saveRandomPlaylist(list) {
  localStorage.setItem('sml-random-playlist', JSON.stringify(list));
}

function handleRandomFXClick(card, wasActive) {
  const cards = $$('.effect-card');

  // SLAVE: toggle simple, sin config ni timer
  if (!SML.isMaster) {
    if (wasActive) {
      showToast('Only the master can configure random mode', 'info');
      return;
    }
    if (SML.randomFXMode) {
      stopRandomFX();
      sendCmd({ action: 'randomFX', state: false });
      SML.effectId = 0;
      sendCmd({ effectId: 0 });
      cards.forEach(c => c.classList.remove('active'));
    } else {
      sendCmd({ action: 'randomConfig', mode: 'all', duration: 8, effectPool: RANDOM_FX_POOL, start: true });
      SML.randomFXMode = true;
      SML._playlistIndex = 0;
      cards.forEach(c => c.classList.remove('active'));
      card.classList.add('active');
    }
    updatePeekEffectInfo();
    return;
  }

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

    // Iniciar random FX via ESP32 — siempre en modo 'all' desde el click.
    // Si el usuario quiere category/playlist, segundo click → config.
    // (localStorage no debe interferir: un valor previo 'playlist' evitaría
    //  la activación directa y abriría config, lo cual el usuario NO espera)
    sendCmd({
      action: 'randomConfig',
      mode: 'all',
      duration: parseInt(localStorage.getItem('sml-random-duration') || '8'),
      effectPool: RANDOM_FX_POOL,
      start: true
    });
    SML.randomFXMode = true;
    SML._playlistIndex = 0;
    cards.forEach(c => c.classList.remove('active'));
    card.classList.add('active');
    updatePeekEffectInfo();
  }
}

function handleRandomVUClick(card, wasActive) {
  const cards = $$('.effect-card');

  // SLAVE: toggle simple, sin config ni timer
  if (!SML.isMaster) {
    if (wasActive) {
      showToast('Only the master can configure random mode', 'info');
      return;
    }
    if (SML.randomVUMode) {
      stopRandomVU();
      sendCmd({ action: 'randomVU', state: false });
      SML.effectId = 0;
      sendCmd({ effectId: 0 });
      cards.forEach(c => c.classList.remove('active'));
    } else {
      sendCmd({
        action: 'randomVU',
        state: true,
        duration: getRandomVUDurationSeconds(),
        effectPool: RANDOM_VU_POOL
      });
      SML.randomVUMode = true;
      cards.forEach(c => c.classList.remove('active'));
      card.classList.add('active');
    }
    updatePeekEffectInfo();
    return;
  }

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

    // Iniciar random VU — ESP32 gestiona el cycling internamente
    sendCmd({
      action: 'randomVU',
      state: true,
      duration: getRandomVUDurationSeconds(),
      effectPool: RANDOM_VU_POOL
    });
    SML.randomVUMode = true;
    cards.forEach(c => c.classList.remove('active'));
    card.classList.add('active');
    updatePeekEffectInfo();
  }
}


function highlightActiveCard(effId) {
  // Immediate visual feedback — no espera la respuesta WS
  $$('.effect-card').forEach(c => c.classList.remove('active'));
  document.querySelectorAll(`.effect-card[data-effect-id="${effId}"]`).forEach(c => c.classList.add('active'));
}

function scrollPlaylistTo(effId) {
  const row = document.getElementById('playlistScrollRow');
  if (!row) return;
  const card = row.querySelector(`.effect-card[data-effect-id="${effId}"]`);
  if (card) {
    card.scrollIntoView({ behavior: 'smooth', block: 'nearest', inline: 'center' });
  }
}

function scrollToCategoryCard(effId) {
  const card = document.querySelector(
    `.effect-category:not(#catFavorites):not(#catPlaylist) .effect-card[data-effect-id="${effId}"]`
  );
  if (card) {
    card.scrollIntoView({ behavior: 'smooth', block: 'nearest', inline: 'center' });
  }
}

function highlightCategories(catIds) {
  document.querySelectorAll('.category-header').forEach(h => h.classList.remove('active'));
  if (catIds && catIds.length) {
    catIds.forEach(catId => {
      const cat = document.getElementById(catId);
      if (cat) {
        const header = cat.querySelector('.category-header');
        if (header) header.classList.add('active');
      }
    });
  }
}


function stopRandomFX() {
  SML.randomFXMode = false;
  if (SML._randomFXTimer) {
    clearTimeout(SML._randomFXTimer);
    SML._randomFXTimer = null;
  }
  // Quitar active de todas las tarjetas en scroll rows (playlist + categorías)
  document.querySelectorAll('.scroll-row .effect-card.active').forEach(c => c.classList.remove('active'));
  // Limpiar highlight de categorías
  highlightCategories(null);
}

function stopRandomVU() {
  SML.randomVUMode = false;
  if (SML._randomVUTimer) {
    clearTimeout(SML._randomVUTimer);
    SML._randomVUTimer = null;
  }
}

// ============================================================================
// EFFECT CONFIG PANELS (offcanvas + bottom sheet)
// ============================================================================

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
  // SLAVE: no puede configurar random mode
  if ((effId === 99 || effId === 100) && !SML.isMaster) {
    showToast('Only the master can configure random mode', 'info');
    return;
  }
  // Random FX — full config with 3 modes
  if (effId === 99) {
    showRandomFXConfig(cardEl);
    return;
  }
  // Random VU — simplified duration config
  if (effId === 100) {
    showRandomDurationConfig(cardEl);
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

function showRandomDurationConfig(cardEl) {
  const mode = window.innerWidth < 768 ? 'sheet' : 'offcanvas';
  const body = document.getElementById(mode === 'sheet' ? 'paramSheetBody' : 'effectOffcanvasBody');
  const title = document.getElementById(mode === 'sheet' ? 'paramSheetTitle' : 'effectOffcanvasTitle');
  const container = document.getElementById(mode === 'sheet' ? 'paramBottomSheet' : 'effectOffcanvas');
  const overlay = document.getElementById(mode === 'sheet' ? 'paramModalOverlay' : 'offcanvasOverlay');

  if (!body || !title || !container) return;
  title.textContent = 'Random VU';
  const curVal = String(getRandomVUDurationSeconds());
  body.innerHTML = `
    <div class="param-row">
      <label>Duration (seconds)</label>
      <input type="range" id="randomDurationInline" min="3" max="30" value="${curVal}" step="1">
      <span class="param-value" id="randomDurationInlineVal">${curVal}s</span>
    </div>
    <div class="palette-section-title" style="margin-top:12px"><span class="fas fa-info-circle"></span> About</div>
    <p style="font-size:0.8rem;color:var(--text-secondary);padding:8px;line-height:1.5">
      Each VU effect plays for the set duration, then randomly switches.
    </p>
  `;
  const slider = body.querySelector('#randomDurationInline');
  const valEl = body.querySelector('#randomDurationInlineVal');
  if (slider && valEl) {
    slider.addEventListener('input', () => {
      valEl.textContent = slider.value + 's';
      localStorage.setItem('sml-random-vu-duration', slider.value);
    });
    slider.addEventListener('change', () => {
      const duration = parseInt(slider.value, 10);
      localStorage.setItem('sml-random-vu-duration', String(duration));
      if (SML.randomVUMode) {
        sendCmd({ action: 'randomVUConfig', duration });
      }
    });
  }
  container.classList.add('open');
  if (overlay) overlay.classList.add('open');
}

function showRandomFXConfig(cardEl) {
  const mode = window.innerWidth < 768 ? 'sheet' : 'offcanvas';
  const body = document.getElementById(mode === 'sheet' ? 'paramSheetBody' : 'effectOffcanvasBody');
  const title = document.getElementById(mode === 'sheet' ? 'paramSheetTitle' : 'effectOffcanvasTitle');
  const container = document.getElementById(mode === 'sheet' ? 'paramBottomSheet' : 'effectOffcanvas');
  const overlay = document.getElementById(mode === 'sheet' ? 'paramModalOverlay' : 'offcanvasOverlay');

  if (!body || !title || !container) return;
  title.textContent = 'Random FX';

  const curDuration = localStorage.getItem('sml-random-duration') || '8';
  const curMode = localStorage.getItem('sml-random-mode') || 'all';
  const curCats = getRandomCategories();
  const needsOk = curMode !== 'all' && SML.randomFXMode;

  body.innerHTML = `
    <div class="param-row">
      <label>Duration (seconds)</label>
      <input type="range" id="randomDurationSlider" min="3" max="30" value="${curDuration}" step="1">
      <span class="param-value" id="randomDurationVal">${curDuration}s</span>
    </div>

    <div class="palette-section-title">Mode</div>
    <div class="mode-pills">
      <button class="mode-pill${curMode === 'all' ? ' active' : ''}" data-mode="all">All</button>
      <button class="mode-pill${curMode === 'category' ? ' active' : ''}" data-mode="category">Category</button>
      <button class="mode-pill${curMode === 'playlist' ? ' active' : ''}" data-mode="playlist">Playlist</button>
    </div>

    <div id="randomCatSection" class="random-section" style="display:${curMode === 'category' ? 'block' : 'none'}">
      <div class="palette-section-title">Categories</div>
      ${RANDOM_CATEGORIES.map(c => `<label class="category-checkbox">
        <input type="checkbox" data-cat-id="${c.id}"${curCats.includes(c.id) ? ' checked' : ''}>
        <span>${c.name} (${c.effectIds.length})</span>
      </label>`).join('')}
    </div>

    <div id="randomPlaylistSection" class="random-section" style="display:${curMode === 'playlist' ? 'block' : 'none'}">
      <div class="palette-section-title">Pick effects</div>
      <div id="playlistPicker" class="playlist-picker">${renderPlaylistPicker()}</div>
      <button id="clearPlaylistBtn" style="margin-top:10px;width:100%;padding:8px 14px;border-radius:8px;border:1px solid var(--border);background:color-mix(in srgb,var(--bg-card) 50%,transparent);color:var(--text-secondary);cursor:pointer;font-size:0.78rem">Clear playlist</button>
    </div>

    <div id="randomOkSection" style="display:${curMode === 'all' ? 'none' : 'block'};margin-top:18px">
      <button id="randomOkBtn" class="random-ok-btn">▶ Play</button>
    </div>
  `;

  // Duration slider
  const s = body.querySelector('#randomDurationSlider');
  const v = body.querySelector('#randomDurationVal');
  if (s && v) {
    s.addEventListener('input', () => {
      v.textContent = s.value + 's';
      localStorage.setItem('sml-random-duration', s.value);
    });
  }

  // Mode pills
  body.querySelectorAll('.mode-pill').forEach(p => {
    p.addEventListener('click', () => {
      const newMode = p.dataset.mode;
      body.querySelectorAll('.mode-pill').forEach(x => x.classList.remove('active'));
      p.classList.add('active');
      localStorage.setItem('sml-random-mode', newMode);
      document.getElementById('randomCatSection').style.display = newMode === 'category' ? 'block' : 'none';
      const plSec = document.getElementById('randomPlaylistSection');
      plSec.style.display = newMode === 'playlist' ? 'block' : 'none';
      const okSection = document.getElementById('randomOkSection');

      if (newMode === 'all') {
        // All: immediate start via ESP32
        if (SML.randomFXMode) {
          stopRandomFX();
          sendCmd({ action: 'randomFX', state: false });
        }
        SML._playlistIndex = 0;
        const durationSlider = document.getElementById('randomDurationSlider');
        const duration = parseInt(durationSlider ? durationSlider.value : '8');
        sendCmd({
          action: 'randomConfig',
          mode: 'all',
          duration: duration,
          effectPool: RANDOM_FX_POOL,
          start: true
        });
        SML.randomFXMode = true;
        okSection.style.display = 'none';
      } else {
        // Category or Playlist: stop and wait for OK
        if (SML.randomFXMode) {
          stopRandomFX();
          sendCmd({ action: 'randomFX', state: false });
        }
        okSection.style.display = 'block';
      }
    });
  });

  // OK / Play button
  const okBtn = body.querySelector('#randomOkBtn');
  if (okBtn) {
    okBtn.addEventListener('click', () => {
      stopRandomFX();
      // Build pool and config based on current mode
      const newMode = (document.querySelector('.mode-pill.active') || {}).dataset?.mode || 'all';
      const durationSlider = document.getElementById('randomDurationSlider');
      const duration = parseInt(durationSlider ? durationSlider.value : '8');
      let pool = [];
      let categories = [];

      if (newMode === 'category') {
        document.querySelectorAll('.category-checkbox input:checked').forEach(cb => {
          const cat = RANDOM_CATEGORIES.find(c => c.id === cb.dataset.catId);
          if (cat) {
            pool.push(...cat.effectIds);
            categories.push(RANDOM_CATEGORIES.indexOf(cat));
          }
        });
      } else if (newMode === 'playlist') {
        pool = getRandomPlaylist();
        if (pool.length === 0) {
          showToast('Add at least one effect to the playlist first', 'warning');
          return;
        }
      } else {
        pool = [...RANDOM_FX_POOL];
      }

      // Send config + start to ESP32
      const msg = {
        action: 'randomConfig',
        mode: newMode,
        duration: duration,
        effectPool: pool.length > 0 ? pool : RANDOM_FX_POOL,
        start: true
      };
      if (categories.length > 0) msg.categories = categories;
      sendCmd(msg);

      SML.randomFXMode = true;
      SML._playlistIndex = 0;
      // Clear card actives
      $$('.effect-card').forEach(c => c.classList.remove('active'));
      const rfCard = document.querySelector('.effect-card[data-effect-id="99"]');
      if (rfCard) rfCard.classList.add('active');
      updatePeekEffectInfo();
      // Close config
      container.classList.remove('open');
      if (overlay) overlay.classList.remove('open');
    });
  }

  // Category checkboxes
  body.querySelectorAll('.category-checkbox input').forEach(cb => {
    cb.addEventListener('change', () => {
      const cats = getRandomCategories();
      const idx = cats.indexOf(cb.dataset.catId);
      if (cb.checked && idx === -1) cats.push(cb.dataset.catId);
      else if (!cb.checked && idx >= 0) cats.splice(idx, 1);
      localStorage.setItem('sml-random-categories', JSON.stringify(cats));
    });
  });

  // Clear playlist button
  const clearBtn = body.querySelector('#clearPlaylistBtn');
  if (clearBtn) {
    clearBtn.addEventListener('click', () => {
      saveRandomPlaylist([]);
      refreshPlaylistUI();
      // Propagar a todos los clientes vía ESP32
      if (SML.isMaster) {
        sendCmd({ action: 'randomConfig', effectPool: [], mode: 'playlist' });
      }
    });
  }

  // Playlist picker clicks
  const pickerEl = body.querySelector('#playlistPicker');
  if (pickerEl) {
    pickerEl.addEventListener('click', e => {
      const btn = e.target.closest('.playlist-pick-btn');
      if (!btn) return;
      const id = parseInt(btn.dataset.effId);
      if (!isNaN(id)) addToPlaylist(id);
    });
  }

  container.classList.add('open');
  if (overlay) overlay.classList.add('open');
}

// ── Playlist helpers ──

function getEffectNames() {
  const names = {};
  document.querySelectorAll('.effect-category:not(#catFavorites):not(#catPlaylist) .effect-card').forEach(card => {
    const id = parseInt(card.dataset.effectId);
    if (isNaN(id) || id === 99) return;
    const name = card.querySelector('.effect-name')?.textContent;
    if (name) names[id] = name;
  });
  return names;
}

function renderPlaylistPicker() {
  const names = getEffectNames();
  const pl = getRandomPlaylist();
  return RANDOM_CATEGORIES.map(cat =>
    cat.effectIds.map(id => {
      const name = names[id];
      if (!name) return '';
      const idx = pl.indexOf(id);
      const picked = idx >= 0;
      return `<button class="playlist-pick-btn${picked ? ' picked' : ''}" data-eff-id="${id}">
        <span class="playlist-pick-num">${picked ? (idx + 1) : '+'}</span>
        ${name}
      </button>`;
    }).join('')
  ).join('');
}

function refreshPlaylistUI() {
  syncPlaylistRow();
  const picker = document.getElementById('playlistPicker');
  if (picker) picker.innerHTML = renderPlaylistPicker();
}

function addToPlaylist(effId) {
  const pl = getRandomPlaylist();
  const idx = pl.indexOf(effId);
  if (idx >= 0) pl.splice(idx, 1); // click again = remove
  else pl.push(effId);               // click once = add
  saveRandomPlaylist(pl);
  refreshPlaylistUI();
}

function removeFromPlaylist(effId) {
  const pl = getRandomPlaylist();
  const idx = pl.indexOf(effId);
  if (idx < 0) return;
  pl.splice(idx, 1);
  saveRandomPlaylist(pl);
  refreshPlaylistUI();
}

// ============================================================================
// EFFECT PARAMS RENDERING
// ============================================================================

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
