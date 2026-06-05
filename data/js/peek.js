/* ──────────────────────────────────────────────────────────────────────────────
   peek.js — SML Peek Tab (Live Canvas Preview)
   Phase 0: Stub with canvas rendering logic.
   Full binary WebSocket streaming comes in Phase 3.
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
    this.liveActive = false;

    // Simulated LED data (demo mode)
    this.ledData = [];
    this.demoPhase = 0;

    // Resize canvas
    this.resize();
  }

  resize() {
    const rect = this.canvas.parentElement.getBoundingClientRect();
    const size = Math.min(rect.width || 320, 400);
    this.canvas.width = size;
    this.canvas.height = size;
    this.render();
  }

  setMode(mode) {
    this.mode = mode;
    this.render();
  }

  setResolution(res) {
    this.resolution = parseInt(res) || 1;
  }

  start() {
    this.running = true;
    this.liveActive = true;
    this.tick();
  }

  stop() {
    this.running = false;
    this.liveActive = false;
  }

  // Receive binary data from WebSocket (Phase 3)
  feedBinary(data) {
    if (data[0] !== 0x4C || data[1] !== 0x02) return;
    const w = data[2];
    const h = data[3];
    const colors = [];
    let idx = 4;
    for (let y = 0; y < h; y++) {
      for (let x = 0; x < w; x++) {
        colors.push({ r: data[idx], g: data[idx + 1], b: data[idx + 2] });
        idx += 3;
      }
    }
    this.ledData = colors;
  }

  // Demo mode: generate fake rainbow
  generateDemo() {
    this.demoPhase = (this.demoPhase + 1) % 360;
    const count = Math.min(this.ledCount, 48);
    this.ledData = [];
    for (let i = 0; i < count; i++) {
      const hue = ((this.demoPhase + i * 15) % 360) / 360;
      const rgb = this.hsvToRgb(hue, 1, 0.8);
      this.ledData.push(rgb);
    }
  }

  hsvToRgb(h, s, v) {
    let r, g, b;
    const i = Math.floor(h * 6);
    const f = h * 6 - i;
    const p = v * (1 - s);
    const q = v * (1 - f * s);
    const t = v * (1 - (1 - f) * s);
    switch (i % 6) {
      case 0: r = v; g = t; b = p; break;
      case 1: r = q; g = v; b = p; break;
      case 2: r = p; g = v; b = t; break;
      case 3: r = p; g = q; b = v; break;
      case 4: r = t; g = p; b = v; break;
      case 5: r = v; g = p; b = q; break;
    }
    return { r: Math.round(r * 255), g: Math.round(g * 255), b: Math.round(b * 255) };
  }

  tick() {
    if (!this.running) return;

    // Auto-stop if peek tab is no longer active or page is hidden
    const peekTab = document.getElementById('tabPeek');
    if (!peekTab || !peekTab.classList.contains('active') || document.hidden) {
      this.running = false;
      return;
    }

    // If no live data, generate demo
    if (this.ledData.length === 0 || !this.liveActive) {
      this.generateDemo();
    }

    this.render();
    setTimeout(() => this.tick(), 50); // ~20fps demo mode
  }

  render() {
    const ctx = this.ctx;
    const w = this.canvas.width;
    const h = this.canvas.height;
    if (!ctx || w === 0) return;

    ctx.clearRect(0, 0, w, h);

    // Dark background
    ctx.fillStyle = '#111';
    ctx.fillRect(0, 0, w, h);

    const data = this.ledData;
    if (!data || data.length === 0) {
      ctx.fillStyle = '#333';
      ctx.font = '14px sans-serif';
      ctx.textAlign = 'center';
      ctx.fillText('No data', w / 2, h / 2);
      return;
    }

    const pad = 8;

    if (this.mode === 'strip') {
      this.renderStrip(ctx, data, w, h, pad);
    } else {
      this.renderCircle(ctx, data, w, h, pad);
    }
  }

  renderStrip(ctx, data, w, h, pad) {
    const count = data.length;
    const spacing = (w - pad * 2) / count;
    const r = Math.max(2, Math.min(spacing * 0.4, 8));
    const cy = h / 2;

    // Horizontal center line
    ctx.strokeStyle = '#222';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(pad, cy);
    ctx.lineTo(w - pad, cy);
    ctx.stroke();

    data.forEach((led, i) => {
      const x = pad + spacing * i + spacing / 2;
      ctx.beginPath();
      ctx.arc(x, cy, r, 0, Math.PI * 2);
      ctx.fillStyle = `rgb(${led.r},${led.g},${led.b})`;
      ctx.fill();

      // Glow effect
      const glow = ctx.createRadialGradient(x, cy, 0, x, cy, r * 2);
      glow.addColorStop(0, `rgba(${led.r},${led.g},${led.b},0.3)`);
      glow.addColorStop(1, `rgba(${led.r},${led.g},${led.b},0)`);
      ctx.fillStyle = glow;
      ctx.beginPath();
      ctx.arc(x, cy, r * 2, 0, Math.PI * 2);
      ctx.fill();
    });
  }

  renderCircle(ctx, data, w, h, pad) {
    const cx = w / 2;
    const cy = h / 2;
    const radius = Math.min(w, h) * 0.35;
    const count = data.length;
    const angleStep = (Math.PI * 2) / count;

    // Glow ring background
    ctx.strokeStyle = '#1a1a2e';
    ctx.lineWidth = radius * 0.4;
    ctx.beginPath();
    ctx.arc(cx, cy, radius, 0, Math.PI * 2);
    ctx.stroke();

    data.forEach((led, i) => {
      const angle = -Math.PI / 2 + angleStep * i;
      const x = cx + Math.cos(angle) * radius;
      const y = cy + Math.sin(angle) * radius;
      const dotR = Math.max(2, radius * 0.06);

      ctx.beginPath();
      ctx.arc(x, y, dotR, 0, Math.PI * 2);
      ctx.fillStyle = `rgb(${led.r},${led.g},${led.b})`;
      ctx.fill();

      // Glow
      const glow = ctx.createRadialGradient(x, y, 0, x, y, dotR * 2.5);
      glow.addColorStop(0, `rgba(${led.r},${led.g},${led.b},0.4)`);
      glow.addColorStop(1, `rgba(${led.r},${led.g},${led.b},0)`);
      ctx.fillStyle = glow;
      ctx.beginPath();
      ctx.arc(x, y, dotR * 2.5, 0, Math.PI * 2);
      ctx.fill();
    });

    // Center hub
    ctx.fillStyle = '#1a1a2e';
    ctx.beginPath();
    ctx.arc(cx, cy, radius * 0.08, 0, Math.PI * 2);
    ctx.fill();

    ctx.fillStyle = '#333';
    ctx.beginPath();
    ctx.arc(cx, cy, 3, 0, Math.PI * 2);
    ctx.fill();
  }
}

// Called from main.js when binary data arrives
function handlePeekBinary(data) {
  if (peek && peek.liveActive) {
    peek.feedBinary(data);
  }
}

// Called from main.js when Peek tab is activated
function initPeek() {
  if (!peek) {
    peek = new PeekRenderer('peekCanvas');
    if (!peek.canvas) return;
  }

  peek.resize();
}

// ============================================================================
// PEEK TAB UI BINDING
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

  // Live toggle
  const peekToggle = document.getElementById('peekToggle');
  if (peekToggle) {
    peekToggle.addEventListener('click', () => {
      if (peek && peek.running) {
        peek.stop();
        peekToggle.textContent = '▶ Start';
        peekToggle.classList.remove('active');
      } else {
        if (!peek) peek = new PeekRenderer('peekCanvas');
        peek.start();
        peekToggle.textContent = '⏸ Stop';
        peekToggle.classList.add('active');
      }
    });
  }

  // Window resize
  window.addEventListener('resize', () => {
    if (peek) peek.resize();
  });

  // Page Visibility — stop render when tab is backgrounded
  document.addEventListener('visibilitychange', () => {
    if (document.hidden && peek) peek.stop();
  });
});
