/* ──────────────────────────────────────────────────────────────────────────────
   peek.js — SML Peek Tab (Live Canvas Preview)
   Streams real LED colors from ESP32 via binary WebSocket frames.
   Format: 0x4C 0x02 <w> <h> <RGB data> (WLED-compatible)
   Displays 33 interpolated LEDs regardless of real strip length.
   ────────────────────────────────────────────────────────────────────────────── */

const VISUAL_LEDS = 33;      // Cuantos LEDs visuales mostramos (strip y círculo)
const PEEK_FPS = 25;          // Mantener en sync con PEEK_FRAME_INTERVAL_MS del ESP32
const PEEK_FRAME_MS = Math.round(1000 / PEEK_FPS); // 40 ms (~25 FPS nominales)
const DRAW_FPS = PEEK_FPS;

// El firmware manda el píxel YA escalado por el brillo global del driver (colores
// reales de la tira). En un canvas eso se ve apagado, porque un LED encendido emite
// mucha más luz que su valor RGB pintado de forma plana. Se aplica un lift
// perceptual (gamma) + un halo aditivo proporcional al brillo: el canvas se parece
// a la tira de verdad y el slider de brillo se nota.
const PEEK_GAMMA = 0.78;
const PEEK_LIFT = (() => {
  const t = new Uint8Array(256);
  for (let i = 0; i < 256; i++) t[i] = Math.round(255 * Math.pow(i / 255, PEEK_GAMMA));
  return t;
})();

let peek = null;

class PeekRenderer {
  constructor(canvasId) {
    this.canvas = document.getElementById(canvasId);
    if (!this.canvas) return;
    this.ctx = this.canvas.getContext('2d', { alpha: true });
    this.mode = 'strip';
    this.ledCount = VISUAL_LEDS;
    this.running = false;
    this.ready = false;         // true after first binary frame arrives
    this.sweeping = false;      // true durante la animación de barrido inicial
    this.sweepProgress = 0;     // 0.0 → 1.0
    this._sweepStart = 0;

    this.ledData = [];          // raw data from ESP32 (24 LEDs)
    this.displayData = [];      // interpolated data (33 LEDs)

    // FPS tracking
    this._fpsHistory = [];
    this._lastDataTime = 0;
    this._lastRenderTime = 0;
    this.fps = 0;
    this._renderTimer = null;
  }

  /* ── Resize canvas to fit container ── */
  resize() {
    const parent = this.canvas.parentElement;
    if (!parent) return;
    const rect = parent.getBoundingClientRect();
    const size = Math.min(Math.max(rect.width || 300, 200), 400);
    this.canvas.width = size;
    this.canvas.height = size;
    this.drawIdle();
  }

  setMode(mode) {
    this.mode = mode;
    if (this.running) this.render();
    else this.drawIdle();
  }

  /* ── Interpolación lineal: N reales → M visuales ── */
  interpolateLEDs(data, targetCount) {
    if (!data || data.length === 0) return [];
    const sourceCount = data.length;
    if (sourceCount === targetCount) return [...data];

    const result = [];
    for (let i = 0; i < targetCount; i++) {
      const pos = (i / targetCount) * sourceCount;
      const idx = Math.floor(pos);
      const frac = pos - idx;
      const next = (idx + 1) % sourceCount;

      result.push({
        r: Math.round(data[idx].r + (data[next].r - data[idx].r) * frac),
        g: Math.round(data[idx].g + (data[next].g - data[idx].g) * frac),
        b: Math.round(data[idx].b + (data[next].b - data[idx].b) * frac),
      });
    }
    return result;
  }

  /* ── Start the render loop ── */
  start() {
    if (this.running) return;
    this.running = true;
    this.sweeping = false;
    this.sweepProgress = 0;
    this._fpsHistory = [];
    this.fps = 0;
    this._lastDataTime = 0;
    this._lastRenderTime = 0;
    this.tick();
  }

  stop() {
    this.running = false;
    this.sweeping = false;
    clearTimeout(this._renderTimer);
    this._renderTimer = null;
    this.ready = false;
    this._lastDataTime = 0;
    this._lastRenderTime = 0;
    this._fpsHistory = [];
    this.fps = 0;
    this.drawIdle();
  }

  /* ── Receive binary data from ESP32 ── */
  feedBinary(data) {
    const bytes = data instanceof ArrayBuffer ? new Uint8Array(data) : data;
    if (bytes[0] !== 0x4C) return;
    const version = bytes[1];
    if (version !== 0x01 && version !== 0x02) return;
    const w = bytes[2];
    const h = bytes[3];
    const colors = [];
    let idx = 4;
    for (let y = 0; y < h; y++) {
      for (let x = 0; x < w; x++) {
        colors.push({ r: bytes[idx], g: bytes[idx + 1], b: bytes[idx + 2] });
        idx += 3;
      }
    }
    this.ledData = colors;
    this.displayData = this.interpolateLEDs(colors, VISUAL_LEDS);
    this._lastDataTime = performance.now();

    if (!this.ready) {
      this.ready = true;
      this.sweeping = true;
      this._sweepStart = performance.now();
      if (this.running) this.render();
    }
  }

  /* ── Render only when new LED data is available ── */
  tick() {
    if (!this.running) return;

    const peekTab = document.getElementById('tabPeek');
    if (!peekTab || !peekTab.classList.contains('active') || document.hidden) {
      this.running = false;
      this.drawIdle();
      return;
    }

    try {
      const now = performance.now();

      // Update sweep progress
      if (this.sweeping && this.ready) {
        this.sweepProgress = Math.min(1, (now - this._sweepStart) / 500);
        if (this.sweepProgress >= 1) this.sweeping = false;
      }

      if (this.ready && this.displayData.length > 0) {
        const hasNewData = this._lastDataTime > this._lastRenderTime;
        const frameIsFresh = now - this._lastDataTime <= PEEK_FRAME_MS * 2;
        if (hasNewData && frameIsFresh) this.render();
      } else {
        this.drawWaiting();
      }
    } catch (e) {
      console.warn('Peek render error:', e);
    }

    this._renderTimer = setTimeout(() => this.tick(), PEEK_FRAME_MS);
  }

  /* ── Waiting state ── */
  drawWaiting() {
    const ctx = this.ctx;
    const w = this.canvas.width;
    const h = this.canvas.height;
    if (!ctx || w === 0) return;

    ctx.clearRect(0, 0, w, h);

    ctx.fillStyle = 'rgba(255,255,255,0.4)';
    ctx.font = `${Math.max(14, w / 20)}px sans-serif`;
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText('⏳ Waiting for LED data...', w / 2, h / 2 - 8);

    ctx.font = `${Math.max(10, w / 30)}px sans-serif`;
    ctx.fillStyle = 'rgba(255,255,255,0.25)';
    ctx.fillText('Touch an effect to turn the strip on', w / 2, h / 2 + 20);
  }

  /* ── Idle state ── */
  drawIdle() {
    const ctx = this.ctx;
    const w = this.canvas.width;
    const h = this.canvas.height;
    if (!ctx || w === 0) return;

    ctx.clearRect(0, 0, w, h);

    ctx.fillStyle = 'rgba(255,255,255,0.45)';
    ctx.font = `${Math.max(14, w / 20)}px sans-serif`;
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText('▶ Press Start to preview', w / 2, h / 2 - 8);

    ctx.font = `${Math.max(11, w / 28)}px sans-serif`;
    ctx.fillStyle = 'rgba(255,255,255,0.2)';
    ctx.fillText('(real-time LED stream from ESP32)', w / 2, h / 2 + 20);
  }

  /* ── Update header overlays (FPS, status dot, effect name) ── */
  updateUI() {
    const fpsEl = document.getElementById('peekFps');
    const dotEl = document.getElementById('peekStreamDot');
    if (fpsEl) {
      fpsEl.textContent = this.ready && this.running ? `${this.fps} FPS` : '';
      fpsEl.classList.toggle('active', this.ready && this.running);
    }
    if (dotEl) {
      dotEl.classList.toggle('active', this.ready && this.running);
    }
  }

  /* ── Main render ── */
  render() {
    const ctx = this.ctx;
    const w = this.canvas.width;
    const h = this.canvas.height;
    if (!ctx || w === 0) return;

    // Clear to transparent — el fondo lo da el wrapper via CSS
    ctx.clearRect(0, 0, w, h);

    const data = this.displayData;
    if (!data || data.length === 0) return;

    const pad = Math.max(8, w * 0.03);

    if (this.mode === 'strip') {
      this.renderStrip(ctx, data, w, h, pad);
    } else {
      this.renderCircle(ctx, data, w, h, pad);
    }

    const now = performance.now();
    if (this._lastRenderTime > 0) {
      const dt = now - this._lastRenderTime;
      if (dt > 0) {
        this._fpsHistory.push(1000 / dt);
        if (this._fpsHistory.length > 20) this._fpsHistory.shift();
        this.fps = Math.round(
          this._fpsHistory.reduce((sum, value) => sum + value, 0) / this._fpsHistory.length
        );
      }
    }
    this._lastRenderTime = now;
    this.updateUI();
  }

  /* ── Strip layout con índices ── */
  renderStrip(ctx, data, w, h, pad) {
    const count = data.length;
    const spacing = (w - pad * 2) / count;
    const r = Math.max(4, spacing * 0.65);
    const cy = h / 2;

    // Glow band behind LEDs
    ctx.fillStyle = 'rgba(255,255,255,0.02)';
    ctx.fillRect(pad, cy - r * 2.5, w - pad * 2, r * 5);

    data.forEach((led, i) => {
      // Skip LEDs during sweep animation
      if (this.sweeping) {
        const threshold = i / count;
        if (threshold > this.sweepProgress) return;
      }

      const x = pad + spacing * i + spacing / 2;

      // Color real del LED con lift perceptual (ver PEEK_LIFT)
      const cr = PEEK_LIFT[led.r], cg = PEEK_LIFT[led.g], cb = PEEK_LIFT[led.b];
      const lum = Math.max(cr, cg, cb) / 255;

      // Halo aditivo en dos capas: núcleo cercano (brillo del LED) + bloom ancho.
      // Se acumula entre LEDs vecinos, así que la tira se lee como luz continua.
      // El halo arranca en el borde del punto (r*0.9), no en su centro: si empieza
      // en el centro se suma encima del propio LED y satura el color a blanco.
      // Alfas contenidas: con 33 LEDs los halos se solapan y un alpha alto lavaba
      // el punto a blanco (el brillo dejaba de distinguirse).
      if (lum > 0.02) {
        ctx.save();
        ctx.globalCompositeOperation = 'lighter';
        const halo = ctx.createRadialGradient(x, cy, r * 0.9, x, cy, r * 3.4);
        halo.addColorStop(0, `rgba(${cr},${cg},${cb},${(0.09 + 0.14 * lum).toFixed(3)})`);
        halo.addColorStop(0.45, `rgba(${cr},${cg},${cb},${(0.03 + 0.06 * lum).toFixed(3)})`);
        halo.addColorStop(1, `rgba(${cr},${cg},${cb},0)`);
        ctx.fillStyle = halo;
        ctx.beginPath();
        ctx.arc(x, cy, r * 3.4, 0, Math.PI * 2);
        ctx.fill();
        const bloom = ctx.createRadialGradient(x, cy, r * 1.1, x, cy, r * 6.5);
        bloom.addColorStop(0, `rgba(${cr},${cg},${cb},${(0.03 + 0.05 * lum).toFixed(3)})`);
        bloom.addColorStop(1, `rgba(${cr},${cg},${cb},0)`);
        ctx.fillStyle = bloom;
        ctx.beginPath();
        ctx.arc(x, cy, r * 6.5, 0, Math.PI * 2);
        ctx.fill();
        ctx.restore();
      }

      // LED dot
      ctx.beginPath();
      ctx.arc(x, cy, r, 0, Math.PI * 2);
      ctx.fillStyle = `rgb(${cr},${cg},${cb})`;
      ctx.fill();

      // Núcleo brillante: aspecto de LED encendido (no un disco plano), sin lavar el color
      if (lum > 0.12) {
        ctx.save();
        ctx.globalCompositeOperation = 'lighter';
        ctx.beginPath();
        ctx.arc(x, cy, r * 0.55, 0, Math.PI * 2);
        ctx.fillStyle = `rgba(255,255,255,${(0.02 + 0.05 * lum).toFixed(3)})`;
        ctx.fill();
        ctx.restore();
      }

      // Subtle border
      ctx.strokeStyle = `rgba(255,255,255,${(0.05 + 0.10 * lum).toFixed(3)})`;
      ctx.lineWidth = 0.5;
      ctx.stroke();
    });
  }

  /* ── Circle layout ── */
  renderCircle(ctx, data, w, h, pad) {
    const cx = w / 2;
    const cy = h / 2;
    const radius = Math.min(w, h) * 0.38;
    const count = data.length;
    const angleStep = (Math.PI * 2) / count;

    // Ring background
    ctx.strokeStyle = 'rgba(255,255,255,0.04)';
    ctx.lineWidth = radius * 0.4;
    ctx.beginPath();
    ctx.arc(cx, cy, radius, 0, Math.PI * 2);
    ctx.stroke();

    data.forEach((led, i) => {
      // Skip LEDs during sweep animation
      if (this.sweeping) {
        const threshold = i / count;
        if (threshold > this.sweepProgress) return;
      }

      const angle = -Math.PI / 2 + angleStep * i;
      const x = cx + Math.cos(angle) * radius;
      const y = cy + Math.sin(angle) * radius;
      const dotR = Math.max(3, radius * 0.07);

      // Mismo lift + halo aditivo que la vista de tira
      const cr = PEEK_LIFT[led.r], cg = PEEK_LIFT[led.g], cb = PEEK_LIFT[led.b];
      const lum = Math.max(cr, cg, cb) / 255;

      if (lum > 0.02) {
        ctx.save();
        ctx.globalCompositeOperation = 'lighter';
        const halo = ctx.createRadialGradient(x, y, dotR * 0.9, x, y, dotR * 3.6);
        halo.addColorStop(0, `rgba(${cr},${cg},${cb},${(0.09 + 0.14 * lum).toFixed(3)})`);
        halo.addColorStop(0.45, `rgba(${cr},${cg},${cb},${(0.03 + 0.06 * lum).toFixed(3)})`);
        halo.addColorStop(1, `rgba(${cr},${cg},${cb},0)`);
        ctx.fillStyle = halo;
        ctx.beginPath();
        ctx.arc(x, y, dotR * 3.6, 0, Math.PI * 2);
        ctx.fill();
        const bloom = ctx.createRadialGradient(x, y, dotR * 1.1, x, y, dotR * 6.5);
        bloom.addColorStop(0, `rgba(${cr},${cg},${cb},${(0.03 + 0.05 * lum).toFixed(3)})`);
        bloom.addColorStop(1, `rgba(${cr},${cg},${cb},0)`);
        ctx.fillStyle = bloom;
        ctx.beginPath();
        ctx.arc(x, y, dotR * 6.5, 0, Math.PI * 2);
        ctx.fill();
        ctx.restore();
      }

      // LED dot
      ctx.beginPath();
      ctx.arc(x, y, dotR, 0, Math.PI * 2);
      ctx.fillStyle = `rgb(${cr},${cg},${cb})`;
      ctx.fill();

      // Núcleo brillante
      if (lum > 0.12) {
        ctx.save();
        ctx.globalCompositeOperation = 'lighter';
        ctx.beginPath();
        ctx.arc(x, y, dotR * 0.55, 0, Math.PI * 2);
        ctx.fillStyle = `rgba(255,255,255,${(0.02 + 0.05 * lum).toFixed(3)})`;
        ctx.fill();
        ctx.restore();
      }

      ctx.strokeStyle = `rgba(255,255,255,${(0.06 + 0.10 * lum).toFixed(3)})`;
      ctx.lineWidth = 0.5;
      ctx.stroke();

      // Pin 1 marker (LED 0)
      if (i === 0) {
        const pinDist = radius + dotR + 6;
        const px = cx + Math.cos(angle) * pinDist;
        const py = cy + Math.sin(angle) * pinDist;
        ctx.beginPath();
        ctx.arc(px, py, 2.5, 0, Math.PI * 2);
        ctx.fillStyle = 'rgba(255,255,255,0.3)';
        ctx.fill();
      }
    });

    // Center hub
    ctx.fillStyle = 'rgba(255,255,255,0.04)';
    ctx.beginPath();
    ctx.arc(cx, cy, radius * 0.06, 0, Math.PI * 2);
    ctx.fill();
  }
}

/* ── Called from main.js when binary ESP32 data arrives ── */
function handlePeekBinary(data) {
  const bytes = data instanceof ArrayBuffer ? new Uint8Array(data) : data;
  if (peek && peek.running) {
    peek.feedBinary(bytes);
  }
}

/* ── Called from main.js when Peek tab is activated ── */
function initPeek() {
  if (!peek) {
    peek = new PeekRenderer('peekCanvas');
    if (!peek.canvas) return;
  }
  peek.resize();
}

// ============================================================================
// UI BINDING
// ============================================================================

document.addEventListener('DOMContentLoaded', () => {

  // Mode buttons
  document.querySelectorAll('[data-peek-mode]').forEach(btn => {
    btn.addEventListener('click', () => {
      document.querySelectorAll('[data-peek-mode]').forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      if (peek) peek.setMode(btn.dataset.peekMode);
    });
  });

  // ── Modo de color del Peek ──────────────────────────────────────────────
  // ON  = colores verdaderos: el firmware escala cada píxel con el brillo y la
  //       corrección del driver, así que el canvas refleja la tira real.
  // OFF = colores del efecto sin corrección de color ni temperatura. El brillo
  //       global SÍ se aplica en los dos modos (si no, al bajar el slider el
  //       preview seguía a tope).
  const TRUE_COLORS_KEY = 'sml-peek-true-colors';
  let trueColors = localStorage.getItem(TRUE_COLORS_KEY) !== '0';
  const colorModeBtn = document.getElementById('peekColorMode');

  function applyColorMode(notify) {
    if (colorModeBtn) {
      colorModeBtn.classList.toggle('active', trueColors);
      colorModeBtn.title = trueColors
        ? 'True colors: exact strip output — brightness, color correction and temperature (tap to see the effect colors without correction)'
        : 'Effect colors without correction: brightness still applies, no color correction or temperature (tap for true colors)';
      colorModeBtn.setAttribute('aria-pressed', trueColors ? 'true' : 'false');
    }
    if (notify && typeof sendCmd === 'function') {
      sendCmd({ action: 'peekColorMode', real: trueColors });
    }
  }

  if (colorModeBtn) {
    colorModeBtn.addEventListener('click', () => {
      trueColors = !trueColors;
      localStorage.setItem(TRUE_COLORS_KEY, trueColors ? '1' : '0');
      applyColorMode(true);
    });
    applyColorMode(false);   // pinta el estado guardado; el valor se manda al arrancar
  }

  // Start / Stop toggle
  const peekToggle = document.getElementById('peekToggle');
  if (peekToggle) {
    peekToggle.addEventListener('click', () => {
      if (peek && peek.running) {
        peek.stop();
        peekToggle.textContent = '▶ Start';
        peekToggle.classList.remove('active');
        if (typeof sendCmd === 'function') sendCmd({ lv: false });
      } else {
        // Sin gate de encendido: el Peek se puede abrir con la tira apagada
        // (mientras no haya efecto activo el frame llega negro).
        if (!peek) peek = new PeekRenderer('peekCanvas');
        peek.start();
        peekToggle.textContent = '⏸ Stop';
        peekToggle.classList.add('active');
        // real = modo de color elegido, para que el stream arranque ya con él
        if (typeof sendCmd === 'function') sendCmd({ lv: true, real: trueColors });
      }
    });
  }

  // Window resize
  window.addEventListener('resize', () => {
    if (peek) peek.resize();
  });

  // Page visibility
  document.addEventListener('visibilitychange', () => {
    if (document.hidden && peek && peek.running) {
      peek.stop();
      if (typeof sendCmd === 'function') sendCmd({ lv: false });
      if (peekToggle) {
        peekToggle.textContent = '▶ Start';
        peekToggle.classList.remove('active');
      }
    }
  });
});

/* ── Firmware says another client currently owns the single Peek stream ── */
function handlePeekBusy() {
  if (peek) peek.stop();
  const peekToggle = document.getElementById('peekToggle');
  if (peekToggle) {
    peekToggle.textContent = '▶ Start';
    peekToggle.classList.remove('active');
  }
  if (typeof showToast === 'function') {
    showToast('Live preview is already running on another device', 'info');
  }
}
