/* ──────────────────────────────────────────────────────────────────────────────
   peek.js — SML Peek Tab (Live Canvas Preview)
   Streams real LED colors from ESP32 via binary WebSocket frames.
   Format: 0x4C 0x02 <w> <h> <RGB data> (WLED-compatible)
   ────────────────────────────────────────────────────────────────────────────── */

let peek = null;

class PeekRenderer {
  constructor(canvasId) {
    this.canvas = document.getElementById(canvasId);
    if (!this.canvas) return;
    this.ctx = this.canvas.getContext('2d');
    this.mode = 'strip';
    this.resolution = 1;
    this.ledCount = 24;
    this.running = false;
    this.ready = false; // true after first binary frame arrives

    this.ledData = [];
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

  setResolution(res) {
    this.resolution = parseInt(res) || 1;
  }

  /* ── Start / stop render loop ── */
  start() {
    if (this.running) return;
    this.running = true;
    this.tick();
  }

  stop() {
    this.running = false;
    this.drawIdle();
  }

  /* ── Receive binary data from ESP32 (WLED format) ── */
  feedBinary(data) {
    // WebSocket binaryType='arraybuffer' → data is ArrayBuffer, wrap in Uint8Array
    const bytes = data instanceof ArrayBuffer ? new Uint8Array(data) : data;
    if (bytes[0] !== 0x4C) return;
    // version 1 = single strip, version 2 = 2D matrix
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
    this.ledCount = w * h;
    if (!this.ready) {
      this.ready = true;
      if (this.running) this.render(); // immediate first frame
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
      if (this.ready && this.ledData.length > 0) {
        this.render();
      } else {
        this.drawWaiting();
      }
    } catch (e) {
      console.warn('Peek render error:', e);
    }

    setTimeout(() => this.tick(), 33);
  }

  /* ── Waiting state (running but no ESP32 data yet) ── */
  drawWaiting() {
    const ctx = this.ctx;
    const w = this.canvas.width;
    const h = this.canvas.height;
    if (!ctx || w === 0) return;

    ctx.fillStyle = '#1a1a2e';
    ctx.fillRect(0, 0, w, h);

    ctx.fillStyle = '#666';
    ctx.font = `${Math.max(14, w / 20)}px sans-serif`;
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText('⏳ Waiting for LED data...', w / 2, h / 2 - 8);

    ctx.font = `${Math.max(10, w / 30)}px sans-serif`;
    ctx.fillStyle = '#444';
    ctx.fillText('Turn on the NeoPixel strip to start streaming', w / 2, h / 2 + 20);
  }

  /* ── Idle state (not running) ── */
  drawIdle() {
    const ctx = this.ctx;
    const w = this.canvas.width;
    const h = this.canvas.height;
    if (!ctx || w === 0) return;

    ctx.fillStyle = '#1a1a2e';
    ctx.fillRect(0, 0, w, h);

    ctx.fillStyle = '#555';
    ctx.font = `${Math.max(14, w / 20)}px sans-serif`;
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText('▶ Press Start to preview', w / 2, h / 2 - 8);

    ctx.font = `${Math.max(11, w / 28)}px sans-serif`;
    ctx.fillStyle = '#333';
    ctx.fillText('(real-time LED stream from ESP32)', w / 2, h / 2 + 20);
  }

  /* ── Render current LED data on canvas ── */
  render() {
    const ctx = this.ctx;
    const w = this.canvas.width;
    const h = this.canvas.height;
    if (!ctx || w === 0) return;

    ctx.fillStyle = '#1a1a2e';
    ctx.fillRect(0, 0, w, h);

    const data = this.ledData;
    if (!data || data.length === 0) return;

    const pad = Math.max(8, w * 0.03);

    if (this.mode === 'strip') {
      this.renderStrip(ctx, data, w, h, pad);
    } else {
      this.renderCircle(ctx, data, w, h, pad);
    }
  }

  /* ── Strip layout ── */
  renderStrip(ctx, data, w, h, pad) {
    const count = data.length;
    const spacing = (w - pad * 2) / count;
    const r = Math.max(3, spacing * 0.35);
    const cy = h / 2;

    // Center line (subtle)
    ctx.strokeStyle = '#2a2a4e';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(pad, cy);
    ctx.lineTo(w - pad, cy);
    ctx.stroke();

    data.forEach((led, i) => {
      const x = pad + spacing * i + spacing / 2;

      // Glow
      const glow = ctx.createRadialGradient(x, cy, 0, x, cy, r * 2.5);
      glow.addColorStop(0, `rgba(${led.r},${led.g},${led.b},0.25)`);
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
    });
  }

  /* ── Circle layout ── */
  renderCircle(ctx, data, w, h, pad) {
    const cx = w / 2;
    const cy = h / 2;
    const radius = Math.min(w, h) * 0.35;
    const count = data.length;
    const angleStep = (Math.PI * 2) / count;

    // Ring background
    ctx.strokeStyle = '#2a2a4e';
    ctx.lineWidth = radius * 0.35;
    ctx.beginPath();
    ctx.arc(cx, cy, radius, 0, Math.PI * 2);
    ctx.stroke();

    data.forEach((led, i) => {
      const angle = -Math.PI / 2 + angleStep * i;
      const x = cx + Math.cos(angle) * radius;
      const y = cy + Math.sin(angle) * radius;
      const dotR = Math.max(3, radius * 0.07);

      // Glow
      const glow = ctx.createRadialGradient(x, y, 0, x, y, dotR * 2.5);
      glow.addColorStop(0, `rgba(${led.r},${led.g},${led.b},0.3)`);
      glow.addColorStop(1, `rgba(${led.r},${led.g},${led.b},0)`);
      ctx.fillStyle = glow;
      ctx.beginPath();
      ctx.arc(x, y, dotR * 2.5, 0, Math.PI * 2);
      ctx.fill();

      // LED dot
      ctx.beginPath();
      ctx.arc(x, y, dotR, 0, Math.PI * 2);
      ctx.fillStyle = `rgb(${led.r},${led.g},${led.b})`;
      ctx.fill();
      ctx.strokeStyle = 'rgba(255,255,255,0.1)';
      ctx.lineWidth = 1;
      ctx.stroke();
    });

    // Center hub
    ctx.fillStyle = '#2a2a4e';
    ctx.beginPath();
    ctx.arc(cx, cy, radius * 0.08, 0, Math.PI * 2);
    ctx.fill();
  }
}

/* ── Called from main.js when binary ESP32 data arrives ── */
function handlePeekBinary(data) {
  // data es ArrayBuffer (por binaryType='arraybuffer'), lo envolvemos en Uint8Array
  const bytes = data instanceof ArrayBuffer ? new Uint8Array(data) : data;
  console.log(`[Peek] Binary frame: ${bytes.byteLength} bytes, ${bytes.length} pixels, sample=RGB(${bytes[4]},${bytes[5]},${bytes[6]})`, bytes);
  if (peek && peek.running) {
    peek.feedBinary(bytes);
  } else {
    console.log(`[Peek] Binary dropped — not running`);
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

  // Resolution dropdown
  const resSelect = document.getElementById('peekResolution');
  if (resSelect) {
    resSelect.addEventListener('change', () => {
      if (peek) peek.setResolution(resSelect.value);
    });
  }

  // Start / Stop toggle
  const peekToggle = document.getElementById('peekToggle');
  if (peekToggle) {
    peekToggle.addEventListener('click', () => {
      if (peek && peek.running) {
        peek.stop();
        peekToggle.textContent = '▶ Start';
        peekToggle.classList.remove('active');
        // Opt-out from live view
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
        // Opt-in to live view (WLED-style)
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
