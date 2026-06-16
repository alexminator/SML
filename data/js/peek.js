/* ──────────────────────────────────────────────────────────────────────────────
   peek.js — SML Peek Tab (Live Canvas Preview)
   Streams real LED colors from ESP32 via binary WebSocket frames.
   Format: 0x4C 0x02 <w> <h> <RGB data> (WLED-compatible)
   Displays 33 interpolated LEDs regardless of real strip length.
   ────────────────────────────────────────────────────────────────────────────── */

const VISUAL_LEDS = 33;      // Cuantos LEDs visuales mostramos (strip y círculo)
const DRAW_FPS = 30;          // Target FPS del render loop

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
    this._lastFrameTime = 0;
    this.fps = 0;
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
    this._lastFrameTime = 0;
    this.tick();
  }

  stop() {
    this.running = false;
    this.sweeping = false;
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

    if (!this.ready) {
      this.ready = true;
      this.sweeping = true;
      this._sweepStart = performance.now();
      if (this.running) this.render();
    }
  }

  /* ── Render loop (~30fps) ── */
  tick() {
    if (!this.running) return;

    const peekTab = document.getElementById('tabPeek');
    if (!peekTab || !peekTab.classList.contains('active') || document.hidden) {
      this.running = false;
      this.drawIdle();
      return;
    }

    try {
      // Update FPS
      const now = performance.now();
      if (this._lastFrameTime > 0) {
        const dt = now - this._lastFrameTime;
        this._fpsHistory.push(1000 / dt);
        if (this._fpsHistory.length > 20) this._fpsHistory.shift();
        this.fps = Math.round(
          this._fpsHistory.reduce((a, b) => a + b, 0) / this._fpsHistory.length
        );
      }
      this._lastFrameTime = now;

      // Update sweep progress
      if (this.sweeping && this.ready) {
        this.sweepProgress = Math.min(1, (now - this._sweepStart) / 500);
        if (this.sweepProgress >= 1) this.sweeping = false;
      }

      if (this.ready && this.displayData.length > 0) {
        this.render();
      } else {
        this.drawWaiting();
      }
    } catch (e) {
      console.warn('Peek render error:', e);
    }

    setTimeout(() => this.tick(), Math.round(1000 / DRAW_FPS));
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
    ctx.fillText('Turn on the NeoPixel strip to start streaming', w / 2, h / 2 + 20);
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

    this.updateUI();
  }

  /* ── Strip layout con índices ── */
  renderStrip(ctx, data, w, h, pad) {
    const count = data.length;
    const spacing = (w - pad * 2) / count;
    const r = Math.max(3, spacing * 0.35);
    const cy = h / 2;

    // Center line (subtle)
    ctx.strokeStyle = 'rgba(255,255,255,0.04)';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(pad, cy);
    ctx.lineTo(w - pad, cy);
    ctx.stroke();

    // Glow band below LEDs
    ctx.fillStyle = 'rgba(255,255,255,0.02)';
    ctx.fillRect(pad, cy - r * 3, w - pad * 2, r * 6);

    data.forEach((led, i) => {
      // Skip LEDs during sweep animation
      if (this.sweeping) {
        const threshold = i / count;
        if (threshold > this.sweepProgress) return;
      }

      const x = pad + spacing * i + spacing / 2;

      // Glow
      const glow = ctx.createRadialGradient(x, cy, 0, x, cy, r * 2.5);
      glow.addColorStop(0, `rgba(${led.r},${led.g},${led.b},0.3)`);
      glow.addColorStop(1, `rgba(${led.r},${led.g},${led.b},0)`);
      ctx.fillStyle = glow;
      ctx.beginPath();
      ctx.arc(x, cy, r * 2.5, 0, Math.PI * 2);
      ctx.fill();

      // LED dot
      ctx.beginPath();
      ctx.arc(x, cy, r, 0, Math.PI * 2);
      ctx.fillStyle = `rgb(${led.r},${led.g},${led.b})`;
      ctx.fill();

      // Subtle border
      ctx.strokeStyle = 'rgba(255,255,255,0.06)';
      ctx.lineWidth = 0.5;
      ctx.stroke();

      // Index number (small, below dot)
      if (spacing > 8) {
        ctx.fillStyle = 'rgba(255,255,255,0.12)';
        ctx.font = `${Math.max(5, spacing * 0.3)}px monospace`;
        ctx.textAlign = 'center';
        ctx.textBaseline = 'top';
        ctx.fillText(i + 1, x, cy + r + 3);
      }
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

      // Glow
      const glow = ctx.createRadialGradient(x, y, 0, x, y, dotR * 3);
      glow.addColorStop(0, `rgba(${led.r},${led.g},${led.b},0.35)`);
      glow.addColorStop(1, `rgba(${led.r},${led.g},${led.b},0)`);
      ctx.fillStyle = glow;
      ctx.beginPath();
      ctx.arc(x, y, dotR * 3, 0, Math.PI * 2);
      ctx.fill();

      // LED dot
      ctx.beginPath();
      ctx.arc(x, y, dotR, 0, Math.PI * 2);
      ctx.fillStyle = `rgb(${led.r},${led.g},${led.b})`;
      ctx.fill();
      ctx.strokeStyle = 'rgba(255,255,255,0.08)';
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
        if (typeof SML !== 'undefined' && !SML.powerOn) {
          if (typeof showToast === 'function') {
            showToast('Turn on the NeoPixel strip first', 'warning');
          }
          return;
        }
        if (!peek) peek = new PeekRenderer('peekCanvas');
        peek.start();
        peekToggle.textContent = '⏸ Stop';
        peekToggle.classList.add('active');
        if (typeof sendCmd === 'function') sendCmd({ lv: true });
      }
    });
  }

  // Window resize
  window.addEventListener('resize', () => {
    if (peek) peek.resize();
  });

  // Page visibility
  document.addEventListener('visibilitychange', () => {
    if (document.hidden && peek) peek.stop();
  });
});
