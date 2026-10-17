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
// QUÉ SE PUEDE AJUSTAR EN CADA EFECTO
// ============================================================================
// No todas las paletas lucen en todos los efectos: cada uno muestrea la paleta de
// una forma distinta. Estas listas son curadas a mano leyendo el render() de cada
// efecto y midiendo la luminancia / los colores de las 27 paletas del firmware:
//   · índice fijo (Sparkle, Scanner, Heartbeat, Hallow Eyes = 0; ICU = 120;
//     Drip = 160): si en ese punto la paleta es casi negro, el efecto desaparece.
//     Sólo quedan las que tienen un color vivo ahí.
//   · rampa de calor (Fire, Sunrise, Candle): sólo paletas cálidas; una azul las
//     convierte en hielo.
//   · recorrido por posición/hue: se quitan las que tienen >18% de negro y los
//     duplicados visuales (Ice≈Aurora, C9≈Sunset…), que lucían igual.
//   · RWB toma 3 colores fijos (0/85/170): sólo paletas con 3 tonos distintos.
// Menos es más: nunca más de 6 para el mismo efecto (2-4 en los de índice fijo).
// La paleta 27 ("Lamp color") no se ofrece aquí: es el color de la lámpara y la
// usan los VU (ver VU_COLOR_EFFECTS).
// ============================================================================
const EFFECT_PALETTES = {
  1:  [18, 2, 8, 3],              // Fire — rampas de calor
  2:  [0, 1, 11, 13, 12],         // Moving Dot
  3:  [0, 7, 12, 11, 1],          // Rainbow Beat
  4:  [21, 12, 11],               // RWB — 3 colores en 0/85/170
  5:  [0, 1, 11, 13, 16, 15],     // Ripple
  6:  [0, 1, 11, 12, 13],         // Bouncing Balls
  7:  [0, 1, 11, 13, 17],         // Juggle
  8:  [0, 1, 11, 13, 17, 12],     // Sinelon
  9:  [0, 1, 11, 13, 16, 17],     // Comet
  10: [0, 1, 11, 13, 17],         // Breath
  11: [0, 1, 11, 13, 15],         // Color Sweep
  20: [0, 1, 11, 12, 13],         // Color Wipe
  21: [0, 1, 11, 13, 17, 12],     // Theater Chase
  22: [0, 1, 11, 13, 15],         // Running Lights
  23: [0, 1, 11, 12, 13, 14],     // Dissolve
  24: [0, 1, 11, 13, 17],         // Dual Scan
  25: [0, 1, 11, 13, 15, 14],     // Fade
  26: [0, 1, 11, 13, 17],         // Meteor — sin Smooth usa el índice 64 fijo
  27: [0, 26, 11, 4],             // Sparkle — índice fijo 0 (rojo/blanco/rosa/verde)
  28: [0, 1, 11, 13, 17],         // BPM
  29: [0, 10, 11, 16, 12],        // Plasma
  30: [0, 1, 11, 12, 13],         // Fireworks
  31: [16, 25, 10],               // Lightning — base fija en 32, todo frío
  32: [0, 7, 11, 12, 1],          // Pride 2015
  33: [0, 5, 13, 11, 12],         // Colorwaves
  34: [5, 16, 13],                // Pacifica — olas
  35: [0, 14, 15, 12, 11, 7],     // TwinkleFOX
  36: [10, 4, 5, 16, 0],          // Aurora — verdes/azules
  37: [0, 1, 11, 12, 13],         // Popcorn
  38: [26, 0, 4, 11],             // Scanner — índice fijo 0 (KITT rojo, verde, rosa)
  39: [19, 25, 11],               // Heartbeat — índice fijo 0 (casi todas son rojo igual)
  40: [20, 4, 10],                // ICU — índice fijo 120 (verde brillante/oscuro/menta)
  41: [24, 2, 18, 3],             // Sunrise — rampa de calor
  42: [22, 5, 20],                // Drip — índice fijo 160 (agua)
  43: [23, 2, 8, 3],              // Candle — rampa de calor
  44: [14, 15, 11, 0, 12],        // Chunchun
  45: [25, 4, 15, 0, 11]          // Hallow Eyes — índice fijo 0 (ojos: blanco/verde/ambar/rojo/rosa)
};

// Efectos VU cuyo ÚNICO ajuste es el color: no eligen paleta, usan la 27
// ("Lamp color"), que sigue a la rueda de color como hace Solid.
const VU_COLOR_EFFECTS = new Set([12, 14, 15, 16, 17, 48]);

// Efectos con colores fijos y sin parámetros: no hay nada que configurar, así
// que no llevan engranaje (Old Skool: verde/rojo semánticos).
const NO_SETTINGS_EFFECTS = new Set([13]);

// Efectos con colores fijos pero con sliders (Noisemeter: verde/amarillo/rojo
// semánticos). Llevan engranaje, pero no selector de paleta.
const NO_PALETTE_EFFECTS = new Set([47]);

// Índice de la paleta "Lamp color" del firmware (sigue al color global).
const PALETTE_LAMP_COLOR = 27;

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

// Marca que el click que sigue a un long-press ya se consumió (abrió ajustes).
// Sin esto, soltar el dedo tras mantener pulsado dispararía además el on/off.
let _suppressCardClick = false;
const LONG_PRESS_MS = 550;

// ── Ajustes de la tarjeta (engranaje / long-press) ───────────────────────────
// Antes esto era el SEGUNDO click en la tarjeta; ahora la tarjeta sólo enciende
// y apaga, y los ajustes tienen su propio gesto.
// ── Sólo el efecto ACTIVO tiene configuración abrible ────────────────────────
// Antes se podía abrir el panel de cualquier tarjeta (engranaje o long-press),
// incluso con otro efecto encendido: te dejaba tocando parámetros de un efecto
// apagado. La marca de "activo" es la clase .active de la tarjeta, que mantienen
// sincronizada activateEffect/highlightActiveCard y el broadcast de estado (y se
// limpia al apagar la tira), así que cubre también Favoritos/Playlist clonados y
// los modos Random (que marcan la tarjeta 99/100).
function isActiveEffectCard(effId, card) {
  if (card && card.classList.contains('active')) return true;
  return document.querySelector(`.effect-card.active[data-effect-id="${effId}"]`) !== null;
}

function showCardConfig(effId, card) {
  if (!card || isNaN(effId)) return;
  // Colores fijos y sin parámetros: no hay panel que abrir (tampoco hay engranaje)
  if (NO_SETTINGS_EFFECTS.has(effId)) {
    showToast('This effect has fixed colors and no settings', 'info');
    return;
  }
  if (!isActiveEffectCard(effId, card)) {
    showToast('Turn this effect on first to see its settings', 'warning');
    return;
  }
  // Los modos Random los configura sólo el master (showEffectConfig lo vuelve a
  // comprobar; aquí se filtra para no abrir el panel a los slaves).
  if (effId !== 99 && effId !== 100 && !SML.isMaster) {
    showToast('Only the master can configure effect parameters', 'info');
    return;
  }
  closeEffectConfig();
  showEffectConfig(effId, card);
}

// ── Apagar: la tira se apaga del todo (ya no vuelve a Solid) ───────────────────
function deactivateEffect() {
  closeEffectConfig();
  SML.powerOn = false;
  sendCmd({ action: 'power', state: false });
  highlightActiveCard(-1);
  updateSolidIcon(SML.r, SML.g, SML.b);
  updatePeekEffectInfo();
}

// ── Engranaje de ajustes en cada tarjeta ─────────────────────────────────────
// Se inyecta por JS (no en los ~46 bloques de index.html) para que también
// viaje en las copias clonadas de Favoritos y Playlist.
function injectCardGears() {
  $$('.effect-card').forEach(card => {
    // Los efectos sin nada configurable no llevan engranaje (y si la tarjeta ya
    // traía uno, p.ej. venía clonada de Favoritos, se retira).
    const effId = parseInt(card.dataset.effectId);
    if (NO_SETTINGS_EFFECTS.has(effId)) {
      const stale = card.querySelector('.effect-gear');
      if (stale) stale.remove();
      return;
    }
    if (card.querySelector('.effect-gear')) return;
    const gear = document.createElement('span');
    gear.className = 'effect-gear';
    // Misma estructura que la estrella (glifo en un span interno) para que el icono
    // quede exactamente a la misma altura y con la misma caja.
    const gearIcon = document.createElement('span');
    gearIcon.className = 'fas fa-gear';
    gearIcon.setAttribute('aria-hidden', 'true');
    gear.appendChild(gearIcon);
    gear.setAttribute('role', 'button');
    gear.setAttribute('tabindex', '0');
    gear.setAttribute('aria-label', 'Effect settings');
    gear.title = 'Settings';
    card.appendChild(gear);
  });
}

function initEffectCards() {
  const container = document.querySelector('.effects-scroll-container');
  if (!container) return;
  let activeEffId = null;

  injectCardGears();

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

    // Tarjeta activa → apagar la tira completamente (no volver a Solid).
    // Los ajustes van por el engranaje o long-press, no por un segundo click.
    // Con un modo Random en marcha la tarjeta activa es la que está sonando en
    // ese instante: ahí el click significa “quiero este efecto”, no “apágalo”.
    if (wasActive && !SML.randomFXMode && !SML.randomVUMode) {
      // Segundo toque sobre la tarjeta activa = apagar la tira. Ya no hay efecto
      // por defecto: se apaga y se queda apagada (no vuelve a Solid). El firmware
      // conserva el effectId, así que volver a tocarla reactiva el mismo efecto.
      sendCmd({ action: 'power', state: false });
      SML.powerOn = false;
      // Ninguna tarjeta activa (todas en gris)
      $$('.effect-card').forEach(c => c.classList.remove('active'));
      activeEffId = null;
      updatePeekEffectInfo();
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
    highlightActiveCard(effId);
    activeEffId = effId;
    updateSolidIcon(SML.r, SML.g, SML.b);
    updatePeekEffectInfo();
  }

  // ── Handler #1: Normal effects (Effects tab inside .effects-scroll-container) ──
  container.addEventListener('click', e => {
    // Click ya consumido por un long-press (abrió el panel de ajustes)
    if (_suppressCardClick) { _suppressCardClick = false; return; }

    // Settings gear — sólo abre el panel del efecto ACTIVO (ver showCardConfig)
    const gear = e.target.closest('.effect-gear');
    if (gear) {
      const gearCard = gear.closest('.effect-card');
      if (gearCard) showCardConfig(parseInt(gearCard.dataset.effectId), gearCard);
      return;
    }

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

    const wasActive = document.querySelector(`.effect-card[data-effect-id="${effId}"].active`) !== null;
    activateEffect(effId, wasActive, card);
  });

  // ── Handler #2: VU effect cards (Music tab inside .effects-grid) ──
  const vuGrid = document.querySelector('#tabMusic .effects-grid');
  if (vuGrid) {
    vuGrid.addEventListener('click', e => {
      if (_suppressCardClick) { _suppressCardClick = false; return; }

      // Settings gear — sólo abre el panel del efecto ACTIVO (ver showCardConfig)
      const gear = e.target.closest('.effect-gear');
      if (gear) {
        const gearCard = gear.closest('.effect-card');
        if (gearCard) showCardConfig(parseInt(gearCard.dataset.effectId), gearCard);
        return;
      }

      const card = e.target.closest('.effect-card');
      if (!card) return;
      const effId = parseInt(card.dataset.effectId);
      if (isNaN(effId)) return;

      // Sin gate de encendido: tocar la tarjeta enciende la tira (el firmware lo
      // hace al fijar el effectId). Ya no hay efecto por defecto.
      const wasActive = document.querySelector(`.effect-card[data-effect-id="${effId}"].active`) !== null;
      activateEffect(effId, wasActive, card);
    });
  }

  // ── Long-press (táctil) → ajustes ──
  // En móvil el engranaje es pequeño: mantener pulsada la tarjeta abre el mismo
  // panel. El click posterior al long-press se descarta (_suppressCardClick).
  attachCardLongPress(container);
  if (vuGrid) attachCardLongPress(vuGrid);
}

/**
 * Mantener pulsada una tarjeta (~550 ms) abre sus ajustes, siempre que sea el
 * efecto activo (showCardConfig lo comprueba y avisa con un toast si no lo es).
 * Sólo táctil: el ratón ya tiene hover + engranaje. Un desplazamiento de la fila
 * cancela el temporizador para no abrir el panel mientras se hace scroll.
 */
function attachCardLongPress(el) {
  let timer = null, startX = 0, startY = 0;

  const cancel = () => {
    if (timer) { clearTimeout(timer); timer = null; }
  };

  el.addEventListener('pointerdown', e => {
    if (e.pointerType === 'mouse') return;
    const card = e.target.closest('.effect-card');
    if (!card) return;
    if (e.target.closest('.fav-star') || e.target.closest('.effect-gear')) return;
    startX = e.clientX;
    startY = e.clientY;
    cancel();
    timer = setTimeout(() => {
      timer = null;
      _suppressCardClick = true;
      showCardConfig(parseInt(card.dataset.effectId), card);
    }, LONG_PRESS_MS);
  });

  el.addEventListener('pointermove', e => {
    if (!timer) return;
    if (Math.abs(e.clientX - startX) > 12 || Math.abs(e.clientY - startY) > 12) cancel();
  });

  el.addEventListener('pointerup', cancel);
  el.addEventListener('pointercancel', cancel);
  el.addEventListener('pointerleave', cancel);
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

  // ── OFF ── Cualquier cliente puede parar el ciclado. Igual que las tarjetas
  // de efecto: click sobre la tarjeta activa = apagar la tira y la configuración
  // del modo vive en el engranaje.
  if (wasActive) {
    stopRandomFX();
    sendCmd({ action: 'randomFX', state: false });
    SML.powerOn = false;
    sendCmd({ action: 'power', state: false });
    cards.forEach(c => c.classList.remove('active'));
    highlightActiveCard(-1);
    updateSolidIcon(SML.r, SML.g, SML.b);
    updatePeekEffectInfo();
    return;
  }

  // SLAVE: toggle simple, sin config ni timer
  if (!SML.isMaster) {
    if (SML.randomFXMode) {
      stopRandomFX();
      sendCmd({ action: 'randomFX', state: false });
      SML.powerOn = false;
      sendCmd({ action: 'power', state: false });
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

  if (SML.randomFXMode) {
    // Ya en random pero wasActive=false (remoto) → detener localmente
    stopRandomFX();
    sendCmd({ action: 'randomFX', state: false });
    // Volver a Solid
    SML.powerOn = false;
    sendCmd({ action: 'power', state: false });
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

  // ── OFF ── Igual que Random FX: click sobre la tarjeta activa apaga el modo y
  // la tira; la config está en el engranaje.
  if (wasActive) {
    stopRandomVU();
    sendCmd({ action: 'randomVU', state: false });
    SML.powerOn = false;
    sendCmd({ action: 'power', state: false });
    cards.forEach(c => c.classList.remove('active'));
    highlightActiveCard(-1);
    updateSolidIcon(SML.r, SML.g, SML.b);
    updatePeekEffectInfo();
    return;
  }

  // SLAVE: toggle simple, sin config ni timer
  if (!SML.isMaster) {
    if (SML.randomVUMode) {
      stopRandomVU();
      sendCmd({ action: 'randomVU', state: false });
      SML.powerOn = false;
      sendCmd({ action: 'power', state: false });
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

  if (SML.randomVUMode) {
    // Ya en random pero wasActive=false (remoto) → detener localmente
    stopRandomVU();
    sendCmd({ action: 'randomVU', state: false });
    // Volver a Solid
    SML.powerOn = false;
    sendCmd({ action: 'power', state: false });
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
  // La rueda de color, si estaba montada en este panel, vuelve a su sitio.
  detachColorWheel();
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
    btn.title = 'Touch an effect to turn the strip on';
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
  // Cualquier render reemplaza el innerHTML del panel: saca antes la rueda.
  detachColorWheel();

  // ── Solid (0) ────────────────────────────────────────────────────────────
  // No está en el registro de efectos: no tiene meta ni parámetros, su único
  // ajuste es la RUEDA DE COLOR. Antes caía en el selector de paletas, que en
  // Solid no hace nada (el color sólido es el color global de la tira).
  if (effId === 0) {
    renderSolidColorConfig(container);
    return;
  }

  // ── VU de color (12, 14, 15, 16, 17, 48) ──────────────────────────────────
  // Estos medidores no usan paletas: el firmware les da la paleta 27, que se
  // reconstruye con el color global. Su panel es la rueda, igual que Solid.
  if (VU_COLOR_EFFECTS.has(effId)) {
    renderVuColorConfig(effId, container);
    return;
  }

  // ── Efectos con colores fijos y sin parámetros (13) ────────────────────────
  if (NO_SETTINGS_EFFECTS.has(effId)) {
    container.innerHTML = '<p class="text-muted" style="font-size:0.8rem;padding:8px 0">This effect uses fixed colors and has no settings.</p>';
    return;
  }

  const config = effectMetaCache[effId];
  const paletteData = window._paletteData || { names: [], swatches: [] };
  const live = liveEffectParams[effId] || {};
  // Hasta que el ESP32 no manda el estado del efecto, la paleta activa es su
  // defecto (el pa= de _meta): así al abrir los ajustes ya aparece marcada la
  // paleta con la que el efecto arranca, sin tener que pulsar «Reset».
  // Si ese defecto no está en la lista curada (efectos sin pa=, que caen en
  // Rainbow), se marca la primera de la lista, que es la que lucimos como
  // defecto para ese efecto.
  const metaDefault = config && config.defaultPalette !== undefined ? config.defaultPalette : 0;
  const curated = EFFECT_PALETTES[effId] || [];
  const fallbackDefault = (curated.length && curated.indexOf(metaDefault) < 0)
    ? curated[0]
    : metaDefault;
  const currentPalette = live.palette !== undefined ? live.palette : fallbackDefault;

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
  // Efectos de colores fijos: no hay paleta que elegir (aunque tengan sliders)
  if (NO_PALETTE_EFFECTS.has(effId)) return '';

  if (!paletteData.names || paletteData.names.length === 0) {
    return '<div class="palette-section"><p class="text-muted" style="font-size:0.75rem;padding:8px 0">Loading palettes...</p></div>';
  }

  // Lista curada por efecto: sólo las paletas que realmente lo hacen lucir
  // (ver EFFECT_PALETTES). Si un efecto no está en la tabla, se mantiene el
  // comportamiento anterior (0-17) para no dejarlo sin selector.
  const curated = EFFECT_PALETTES[effId];
  let displayIndices;
  if (curated) {
    displayIndices = curated.slice();
    // Si la paleta guardada en el ESP32 no está en la lista curada (estado de
    // una versión anterior), se añade para que la UI no oculte cuál está activa.
    if (Number.isInteger(currentPalette) && displayIndices.indexOf(currentPalette) < 0) {
      displayIndices.push(currentPalette);
    }
  } else {
    const cfg = effectMetaCache[effId];
    const effectDefPal = cfg ? cfg.defaultPalette : 0;
    displayIndices = [];
    if (effectDefPal > 17) displayIndices.push(effectDefPal);
    for (let i = 0; i <= 17; i++) displayIndices.push(i);
  }

  let html = '<div class="palette-section">';
  html += '<div class="palette-section-title"><span class="fas fa-palette"></span> Color Palette</div>';
  if (curated) {
    html += '<p class="palette-hint">Only the palettes that work with this effect.</p>';
  }
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
      // Reflejar la elección en la cache: si el broadcast tarda (o no trae la
      // paleta), al reabrir el panel sigue marcada la que acabas de elegir.
      liveEffectParams[effId] = Object.assign({}, liveEffectParams[effId], { palette: idx });
      // Update visual selection
      container.querySelectorAll('.palette-swatch').forEach(s => s.classList.remove('selected'));
      el.classList.add('selected');
      el.querySelector('.palette-check-mark').textContent = '✓';
    });
  });
}

// ── Fetch palette data ────────────────────────────────────────────────────────
// ── El FAB de brillo se aparta mientras hay un panel de efecto abierto ───────
// La clase la pone/quita esta función, que main.js dispara observando la clase
// 'open' de los dos paneles: así el FAB vuelve siempre a su sitio, incluso si
// mañana se añade otra vía de apertura o de cierre.
function syncEffectPanelState() {
  // Los paneles de efecto (offcanvas de escritorio + bottom sheet móvil) y también
  // los modales de Help/About: los FABs tienen que apartarse igual debajo de todos
  // ellos, o quedan flotando encima de la hoja.
  const panels = ['effectOffcanvas', 'paramBottomSheet', 'helpModal', 'aboutModal']
    .map(id => document.getElementById(id));
  const anyOpen = panels.some(el => el && el.classList.contains('open'));
  document.body.classList.toggle('effect-modal-open', anyOpen);
  // El slider del FAB se cierra al abrir: si no, quedaría abierto (oculto) y al
  // volver el FAB aparecería con el slider desplegado.
  if (anyOpen && typeof SML._hideFabSlider === 'function') SML._hideFabSlider();
}

// Engancha el observador que mantiene el estado del FAB al día. Lo llama main.js
// al inicializar la UI; los paneles se buscan por id (offcanvas y bottom sheet).
function initEffectPanelObserver() {
  ['effectOffcanvas', 'paramBottomSheet', 'helpModal', 'aboutModal'].forEach(id => {
    const el = document.getElementById(id);
    if (el) new MutationObserver(syncEffectPanelState).observe(el, { attributes: true, attributeFilter: ['class'] });
  });
  // Estado inicial (si algún panel ya vieniera abierto al cargar).
  syncEffectPanelState();
}

// ============================================================================
// SOLID (0): RUEDA DE COLOR DENTRO DEL PANEL DE AJUSTES
// ============================================================================
// La app tiene UNA sola instancia de iro (la del panel de color, creada en
// controls.js): aquí se MUEVE su host al panel en vez de crear otra rueda, así se
// edita el mismo estado (SML.r/g/b) y no hay que sincronizar dos pickers. El host
// es un nodo normal: moverlo conserva el canvas y sus listeners.
// ============================================================================

function mountColorPicker(container) {
  const host = document.getElementById('colorPicker');
  if (!host || !SML.colorPicker) return null;
  container.appendChild(host);
  return SML.colorPicker;
}

// Devuelve la rueda a su contenedor. Obligatorio antes de reemplazar el innerHTML
// de cualquier panel y al cerrar la configuración.
function detachColorWheel() {
  const host = document.getElementById('colorPicker');
  const home = document.querySelector('.color-picker-container');
  if (!host || !home) return;
  if (host.parentElement !== home) home.appendChild(host);
}

function renderSolidColorConfig(container) {
  container.innerHTML = '';

  const title = document.createElement('div');
  title.className = 'palette-section-title';
  title.innerHTML = '<span class="fas fa-palette"></span> Color';
  container.appendChild(title);

  if (!mountColorPicker(container)) {
    container.innerHTML = '<p class="text-muted" style="font-size:0.8rem;padding:8px 0">Color picker unavailable</p>';
    return;
  }

  const hint = document.createElement('p');
  hint.className = 'solid-color-hint';
  hint.textContent = 'Pick any color you want: Solid paints the whole strip with it. Brightness has its own control.';
  container.appendChild(hint);
}

// ── VU de color: su único ajuste es el color ──────────────────────────────────
// Los VU no tienen paletas: el firmware los pinta con la paleta 27, que se
// reconstruye a partir del color global (el mismo que edita la rueda). Así que
// el panel es la rueda, sin selector de paletas, y el cambio se ve en vivo.
function renderVuColorConfig(effId, container) {
  container.innerHTML = '';

  const live = liveEffectParams[effId] || {};
  if (live.palette !== PALETTE_LAMP_COLOR) {
    sendCmd({ action: 'setParams', effectId: effId, palette: PALETTE_LAMP_COLOR });
    liveEffectParams[effId] = Object.assign({}, live, { palette: PALETTE_LAMP_COLOR });
  }

  const title = document.createElement('div');
  title.className = 'palette-section-title';
  title.innerHTML = '<span class="fas fa-palette"></span> Color';
  container.appendChild(title);

  if (!mountColorPicker(container)) {
    container.innerHTML = '<p class="text-muted" style="font-size:0.8rem;padding:8px 0">Color picker unavailable</p>';
    return;
  }

  const hint = document.createElement('p');
  hint.className = 'solid-color-hint';
  const cfg = effectMetaCache[effId];
  hint.textContent = `${cfg ? cfg.name : 'This VU meter'} uses the lamp color: pick it here and the bars follow. It has no palettes.`;
  container.appendChild(hint);
}

async function fetchPalettes() {
  try {
    const resp = await fetch('/palettes');
    const data = await resp.json();
    window._paletteData = data;
  } catch (e) {
    // Silencioso — reintenta en el próximo render
  }
}
