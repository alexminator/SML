/* ──────────────────────────────────────────────────────────────────────────────
   controls.js — SML Lamp controls: power toggles, color picker, brightness FAB
   Depends on: SML{}, sendCmd (websocket.js), showToast (ui.js),
               initEffectCards/initFavorites (effects.js)
   ────────────────────────────────────────────────────────────────────────────── */

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

  // Initialize favorites from localStorage
  initFavorites();

}

// ── Solid icon: refleja el color del picker solo si está activo ─────
function updateSolidIcon(r, g, b) {
  document.querySelectorAll('.effect-card[data-effect-id="0"]').forEach(card => {
    const el = card.querySelector('.effect-icon');
    if (!el) return;
    if (card.classList.contains('active')) {
      el.style.setProperty('--solid-fill', `rgb(${r},${g},${b})`);
    } else {
      el.style.removeProperty('--solid-fill');
    }
  });
}
