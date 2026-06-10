/* =============================================================================
   player.js — Smart Music Lamp · BT Player Controls (simplified)
   =============================================================================
   Botones:
     .play-btn    → Play/Pause  → WS "play-pause"
     .bt-toggle   → BT on/off   → WS "music"

   Sincronización desde WS:
     window.playerSync(data) — actualiza estado del botón play y BT
   ============================================================================= */

(function () {
  'use strict';

  /* ── DOM ──────────────────────────────────────────────────────────────────── */
  const playBtn   = document.querySelector('#playBtn');       // .play-btn
  const btToggle  = document.querySelector('#BTtoggle');      // .bt-toggle

  if (!playBtn || !btToggle) return;

  /* ── Estado ────────────────────────────────────────────────────────────── */
  let playing = false;
  let btOn    = false;

  /* ── WS helper ─────────────────────────────────────────────────────────── */
  function sendWS(action) {
    if (typeof websocket !== 'undefined' && websocket.readyState === WebSocket.OPEN) {
      const msg = Object.assign({}, typeof json !== 'undefined' ? json : {}, { action });
      websocket.send(JSON.stringify(msg));
    } else {
      console.warn('[player] WS not open →', action);
    }
  }

  /* ── Play/Pause ────────────────────────────────────────────────────────── */
  playBtn.addEventListener('click', () => {
    playing = !playing;
    playBtn.classList.toggle('playing', playing);
    sendWS('play-pause');
  });

  /* ── Bluetooth toggle ──────────────────────────────────────────────────── */
  btToggle.addEventListener('click', () => {
    btOn = !btOn;
    btToggle.classList.toggle('on', btOn);
    sendWS('music');
  });

  /* ── Sincronización desde WS (llamado por main.js) ────────────────────── */
  window.playerSync = function (data) {
    if (data.playing !== undefined) {
      playing = !!data.playing;
      playBtn.classList.toggle('playing', playing);
    }
    if (data.bt_powerState !== undefined) {
      btOn = !!data.bt_powerState;
      btToggle.classList.toggle('on', btOn);
    }
  };

})();
