/* =============================================================================
   player.js — Smart Music Lamp · iPod-style Player Controls
   =============================================================================
   Botones:
     .play-btn      → Play/Pause  → WS "play-pause"
     .skip-left     → Prev Track   → WS "skipL"
     .skip-right    → Next Track   → WS "skipR"
     .vol-up        → Volume Up    → WS "volup"
     .vol-down      → Volume Down  → WS "voldown"
     .bt-toggle     → BT on/off    → WS "music"

   Press animations: las 4 esquinas y el centro del btn-overlay se deforman
   ligeramente al presionar (efecto iPod mecánico original).

   Sincronización desde WS:
     window.playerSync(data) — actualiza estado BT
   ============================================================================= */

(function () {
  'use strict';

  /* ── DOM ──────────────────────────────────────────────────────────────────── */
  const playBtn    = document.querySelector('#playBtn');
  const btToggle   = document.querySelector('#BTtoggle');
  const skipLeft   = document.querySelector('.skip-left');
  const skipRight  = document.querySelector('.skip-right');
  const volUp      = document.querySelector('.vol-up');
  const volDown    = document.querySelector('.vol-down');
  const overlay    = document.querySelector('.btn-overlay');   // ← para animations

  if (!playBtn || !btToggle || !overlay) return;

  /* ── Estado ────────────────────────────────────────────────────────────── */
  let btOn = false;

  /* ── WS helper ─────────────────────────────────────────────────────────── */
  function sendWS(action) {
    if (typeof websocket !== 'undefined' && websocket.readyState === WebSocket.OPEN) {
      const msg = Object.assign({}, typeof json !== 'undefined' ? json : {}, { action });
      websocket.send(JSON.stringify(msg));
    } else {
      console.warn('[player] WS not open →', action);
    }
  }

  /* ── Press animation helper (como la web anterior) ───────────────────────
       Las 4 esquinas deforman el btn-overlay; el centro (play) escala el botón.
       ──────────────────────────────────────────────────────────────────────── */
  function addPressEffect(element, className, target) {
    if (!element) return;
    const el = target || overlay;
    element.addEventListener('pointerdown', () => el.classList.add(className));
    element.addEventListener('pointerup',   () => el.classList.remove(className));
    element.addEventListener('pointerleave', () => el.classList.remove(className));
  }

  /* ── Configurar botón: click → WS + press animation ───────────────────── */
  function setupButton(element, action, className, target) {
    if (!element) return;
    element.addEventListener('click', () => sendWS(action));
    addPressEffect(element, className, target);
  }

  /* ── Configurar controles (como la web anterior) ─────────────────────────
       – Las 4 esquinas → btn-overlay (left/right/up/down)
       – El centro (play) → play-btn (pressed)
       ──────────────────────────────────────────────────────────────────────── */
  setupButton(playBtn,   'play-pause', 'pressed', playBtn);
  setupButton(skipLeft,  'skipL',      'left');
  setupButton(skipRight, 'skipR',      'right');
  setupButton(volUp,     'volup',      'up');
  setupButton(volDown,   'voldown',    'down');

  /* ── Bluetooth toggle (mantiene .on class) ─────────────────────────────── */
  btToggle.addEventListener('click', () => {
    btOn = !btOn;
    btToggle.classList.toggle('on', btOn);
    sendWS('music');
  });

  /* ── Sincronización desde WS (llamado por main.js) ────────────────────── */
  window.playerSync = function (data) {
    if (data.bt_powerState !== undefined) {
      btOn = !!data.bt_powerState;
      btToggle.classList.toggle('on', btOn);
    }
  };

})();
