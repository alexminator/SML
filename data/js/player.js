/* =============================================================================
   player.js — Smart Music Lamp · BT Player Controls
   =============================================================================
   Botones (estructura idéntica al CodePen billyysea/nPmEEq):
     .big          → Play/Pause   → WS "play-pause"
     .small.left   → Rewind       → WS "skipL"  + tilt animado en .big
     .small.right  → Fast-forward → WS "skipR"  + tilt animado en .big
     .star-tab     → BT on/off    → WS "music"

   Volumen (anillo .scrub-bg / .scrub-arc):
     - Drag circular dividido en 15 pasos
     - Cada paso = 1 pulsación de volumen al módulo BT
     - Al soltar: calcula delta = newStep − oldStep
       → envía "volup" ó "voldown" |delta| veces con 220ms entre cada una
     - Derecha/arriba = sube volumen · Izquierda/abajo = baja
   =============================================================================
   Arc math  (r=45, viewBox 100×100):
     C       = 2π×45 ≈ 282.74 px
     Visible = 270°  = 75% C ≈ 211.95 px
     Hueco   = 90°   = 25% C ≈  70.69 px  (queda abajo con rotate 135°)
     Por paso: 211.95 / 15 ≈ 14.13 px
     dashoffset(step) = 282.74 − step × 14.13
       step  0 → 282.74  (vacío)
       step 15 →  70.69  (lleno 270°)
     Thumb angle = 225° + step × 18°  (225° = inicio real del arco tras rotate 135°)
   ============================================================================= */

(function () {
  'use strict';

  /* ── DOM ──────────────────────────────────────────────────────────────────── */
  const bigBtn    = document.querySelector('#playBtn');         // .big
  const skipL     = document.querySelector('#skipLeft');        // .small.left
  const skipR     = document.querySelector('#skipRight');       // .small.right
  const starTab   = document.querySelector('#btStarTab');       // .star-tab
  const scrubBg   = document.querySelector('#scrubBg');         // .scrub-bg
  const scrubArc  = document.querySelector('#scrubArc');        // arco SVG
  const scrubThumb= document.querySelector('#scrubThumb');      // dot SVG
  const scrubLabel= document.querySelector('#scrubLabel');      // nº paso

  if (!bigBtn) return; // guard: tab music no montado aún

  /* ── Constantes del arco ───────────────────────────────────────────────── */
  const STEPS      = 15;
  const C          = 282.74;          // circunferencia
  const ARC_PX     = 211.95;          // 270° en px
  const GAP_OFFSET = 70.69;           // dashoffset cuando step=15 (lleno)
  const STEP_PX    = ARC_PX / STEPS;  // ≈ 14.13
  const START_DEG  = 225;             // ángulo de inicio del arco (desde arriba)
  const DEG_STEP   = 270 / STEPS;     // 18° por paso
  const CX = 50, CY = 50, R = 45;    // centro y radio del SVG

  /* ── Estado ────────────────────────────────────────────────────────────── */
  let volStep  = 7;      // 0–15, arranca en el medio
  let btOn     = false;
  let playing  = false;

  /* ── WS helper ─────────────────────────────────────────────────────────── */
  const PRESS_GAP = 220; // ms — coincide con short_delay del firmware

  function sendWS(action) {
    if (typeof websocket !== 'undefined' && websocket.readyState === WebSocket.OPEN) {
      const msg = Object.assign({}, typeof json !== 'undefined' ? json : {}, { action });
      websocket.send(JSON.stringify(msg));
    } else {
      console.warn('[player] WS not open →', action);
    }
  }

  /** Envía `action` exactamente `n` veces con PRESS_GAP ms entre cada una */
  function sendBurst(action, n) {
    if (n < 1) return;
    let sent = 0;
    const id = setInterval(() => {
      sendWS(action);
      if (++sent >= n) clearInterval(id);
    }, PRESS_GAP);
  }

  /* ── Render del anillo ─────────────────────────────────────────────────── */
  function stepToDashoffset(s) {
    return C - s * STEP_PX;          // step 0 → C (vacío), step 15 → GAP_OFFSET
  }

  function stepToThumb(s) {
    const deg = START_DEG + s * DEG_STEP;
    const rad = (deg - 90) * (Math.PI / 180); // 0° = top
    return {
      x: CX + R * Math.cos(rad),
      y: CY + R * Math.sin(rad)
    };
  }

  function renderArc(s, animate) {
    if (!animate) {
      scrubArc.classList.add('no-transition');
      scrubThumb.classList.add('no-transition');
    }
    scrubArc.setAttribute('stroke-dashoffset', stepToDashoffset(s).toFixed(2));
    const { x, y } = stepToThumb(s);
    scrubThumb.setAttribute('cx', x.toFixed(2));
    scrubThumb.setAttribute('cy', y.toFixed(2));
    scrubLabel.textContent = s;
    if (!animate) {
      // forzar reflow para que no-transition surta efecto
      scrubArc.getBoundingClientRect();
      requestAnimationFrame(() => {
        scrubArc.classList.remove('no-transition');
        scrubThumb.classList.remove('no-transition');
      });
    }
  }

  /* ── Drag circular ─────────────────────────────────────────────────────── */
  let dragging      = false;
  let stepAtStart   = volStep;

  /** Convierte posición del puntero en número de paso 0–15 */
  function pointerToStep(e) {
    const rect = scrubBg.getBoundingClientRect();
    const cx   = rect.left + rect.width  / 2;
    const cy   = rect.top  + rect.height / 2;
    const clientX = e.touches ? e.touches[0].clientX : e.clientX;
    const clientY = e.touches ? e.touches[0].clientY : e.clientY;

    // Ángulo relativo al top del círculo (0° = arriba, creciente en sentido horario)
    let deg = Math.atan2(clientY - cy, clientX - cx) * (180 / Math.PI) + 90;
    if (deg < 0) deg += 360;

    // Restar START_DEG para mapear a coordenadas del arco (0 = inicio del arco)
    let rel = deg - START_DEG;
    if (rel < 0) rel += 360;

    // Si cae en la zona de hueco (270°–360°), clampar al extremo más cercano
    if (rel > 270) rel = rel > 315 ? 0 : 270;

    return Math.round((rel / 270) * STEPS);
  }

  scrubBg.addEventListener('pointerdown', (e) => {
    dragging     = true;
    stepAtStart  = volStep;
    scrubBg.classList.add('dragging');
    scrubBg.setPointerCapture(e.pointerId);
    e.preventDefault();
  }, { passive: false });

  scrubBg.addEventListener('pointermove', (e) => {
    if (!dragging) return;
    const s = Math.min(STEPS, Math.max(0, pointerToStep(e)));
    if (s !== volStep) {
      volStep = s;
      renderArc(volStep, false); // sin animación durante drag: instantáneo
    }
  }, { passive: false });

  scrubBg.addEventListener('pointerup', () => {
    if (!dragging) return;
    dragging = false;
    scrubBg.classList.remove('dragging');
    const delta = volStep - stepAtStart;
    if (delta !== 0) {
      sendBurst(delta > 0 ? 'volup' : 'voldown', Math.abs(delta));
    }
  });

  scrubBg.addEventListener('pointercancel', () => {
    dragging = false;
    scrubBg.classList.remove('dragging');
  });

  /* ── Play/Pause ────────────────────────────────────────────────────────── */
  bigBtn.addEventListener('click', () => {
    playing = !playing;
    bigBtn.classList.toggle('playing', playing);
    sendWS('play-pause');
  });

  /* ── Rewind + tilt del .big ────────────────────────────────────────────── */
  skipL.addEventListener('click', () => sendWS('skipL'));
  skipL.addEventListener('pointerdown', () => bigBtn.classList.add('left'));
  skipL.addEventListener('pointerup',   () => bigBtn.classList.remove('left'));
  skipL.addEventListener('pointerleave',() => bigBtn.classList.remove('left'));

  /* ── Fast-forward + tilt del .big ─────────────────────────────────────── */
  skipR.addEventListener('click', () => sendWS('skipR'));
  skipR.addEventListener('pointerdown', () => bigBtn.classList.add('right'));
  skipR.addEventListener('pointerup',   () => bigBtn.classList.remove('right'));
  skipR.addEventListener('pointerleave',() => bigBtn.classList.remove('right'));

  /* ── BT Star Tab ───────────────────────────────────────────────────────── */
  starTab.addEventListener('click', () => {
    btOn = !btOn;
    starTab.classList.toggle('on', btOn);
    sendWS('music');
  });

  /* ── Tilt del .big (.left / .right) ───────────────────────────────────── */
  // (las clases ya se aplican arriba en los listeners de skip)

  /* ── Sincronización desde WS (llamado por main.js) ────────────────────── */
  window.playerSync = function (data) {
    if (data.bt_powerState !== undefined) {
      btOn = !!data.bt_powerState;
      starTab.classList.toggle('on', btOn);
    }
    // Si el firmware envía el paso de volumen actual
    if (data.volStep !== undefined) {
      const s = Math.min(STEPS, Math.max(0, parseInt(data.volStep, 10)));
      if (s !== volStep) { volStep = s; renderArc(volStep, true); }
    }
  };

  /* ── Render inicial ────────────────────────────────────────────────────── */
  renderArc(volStep, false);

})();