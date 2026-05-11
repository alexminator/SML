/*=============== Peek LED Preview Renderer ===============*/
/* SML Web Interface v2.1 - Real-time LED Visualization */

// Initialize logger
const peekLogger = createLogger('Peek');

class LEDPreview {
  constructor() {
    // Canvas setup
    this.canvas = document.getElementById('ledPreviewCanvas');
    if (!this.canvas) {
      peekLogger.error('LED canvas not found');
      return;
    }

    this.ctx = this.canvas.getContext('2d');

    // Rendering state
    this.currentMode = 'strip'; // 'strip' or 'circle'
    this.currentLEDCount = 24; // Will be updated from real LED count
    this.realLEDCount = 24; // Actual hardware LED count
    this.leds = []; // Array of RGB values
    this.effectName = 'None';

    // Performance tracking
    this.lastFrameTime = 0;
    this.fps = 0;
    this.frameCount = 0;
    this.fpsUpdateInterval = 1000; // Update FPS display every 1s
    this.lastFPSUpdate = 0;

    // Animation loop
    this.isRunning = false;
    this.animationFrameId = null;

    // Initialize
    this.init();
  }

  init() {
    // Set initial canvas size
    this.resizeCanvas();
    window.addEventListener('resize', () => this.resizeCanvas());

    // Setup mode toggle buttons
    this.setupModeToggle();

    // Setup LED count selector
    this.setupLEDSelector();

    // Start render loop
    this.start();
  }

  resizeCanvas() {
    const wrapper = this.canvas.parentElement;
    const rect = wrapper.getBoundingClientRect();

    // Set canvas resolution (not display size)
    this.canvas.width = rect.width;
    this.canvas.height = this.currentMode === 'strip' ? 80 : rect.width;

    // Re-render immediately
    this.render();
  }

  setupModeToggle() {
    const stripBtn = document.getElementById('modeStrip');
    const circleBtn = document.getElementById('modeCircle');

    if (stripBtn && circleBtn) {
      stripBtn.addEventListener('click', () => this.setMode('strip'));
      circleBtn.addEventListener('click', () => this.setMode('circle'));
    }
  }

  setMode(mode) {
    this.currentMode = mode;

    // Update button states
    const stripBtn = document.getElementById('modeStrip');
    const circleBtn = document.getElementById('modeCircle');

    if (stripBtn && circleBtn) {
      stripBtn.classList.toggle('active', mode === 'strip');
      circleBtn.classList.toggle('active', mode === 'circle');
    }

    // Resize canvas for new mode
    this.resizeCanvas();
  }

  setupLEDSelector() {
    const selector = document.getElementById('ledCountSelect');
    if (!selector) return;

    // Clear existing options
    selector.innerHTML = '';

    // Generate options from real LED count up to 60
    const startCount = Math.min(this.realLEDCount, 60);
    for (let i = startCount; i <= 60; i++) {
      const option = document.createElement('option');
      option.value = i;
      option.textContent = `${i} LEDs`;
      selector.appendChild(option);
    }

    // Set current selection
    selector.value = this.currentLEDCount;

    // Listen for changes
    selector.addEventListener('change', (e) => {
      this.currentLEDCount = parseInt(e.target.value);
      this.updateInfo();
    });
  }

  updateLEDCount(realCount) {
    this.realLEDCount = realCount;

    // Update selector options
    this.setupLEDSelector();

    // If current selection is below new real count, adjust
    if (this.currentLEDCount < Math.min(realCount, 60)) {
      this.currentLEDCount = Math.min(realCount, 60);
    }

    this.updateInfo();
  }

  update(leds, effectName) {
    // Store LED data
    this.leds = leds;
    this.effectName = effectName;

    // Render will be called by animation loop
  }

  render() {
    if (!this.ctx) return;

    // Clear canvas
    this.ctx.fillStyle = '#000';
    this.ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);

    if (this.leds.length === 0) {
      // Show "Waiting for data..." message
      this.ctx.fillStyle = '#666';
    this.ctx.font = '16px Arial';
      this.ctx.textAlign = 'center';
      this.ctx.fillText('Waiting for LED data...', this.canvas.width / 2, this.canvas.height / 2);
      return;
    }

    // Render based on mode
    if (this.currentMode === 'strip') {
      this.renderStrip();
    } else {
      this.renderCircle();
    }
  }

  renderStrip() {
    const ledSpacing = this.canvas.width / this.currentLEDCount;
    const ledSize = Math.min(ledSpacing * 0.8, 12);
    const yOffset = this.canvas.height / 2;

    for (let i = 0; i < this.currentLEDCount; i++) {
      const ledIndex = this.mapLEDIndex(i, this.currentLEDCount, this.leds.length);
      const color = this.leds[ledIndex];

      const x = i * ledSpacing + ledSpacing / 2;
      const y = yOffset;

      // Draw LED glow
      const gradient = this.ctx.createRadialGradient(x, y, 0, x, y, ledSize);
      gradient.addColorStop(0, `rgb(${color.r}, ${color.g}, ${color.b})`);
      gradient.addColorStop(0.5, `rgba(${color.r}, ${color.g}, ${color.b}, 0.5)`);
      gradient.addColorStop(1, 'rgba(0, 0, 0, 0)');

      this.ctx.fillStyle = gradient;
      this.ctx.beginPath();
      this.ctx.arc(x, y, ledSize, 0, Math.PI * 2);
      this.ctx.fill();

      // Draw LED core
      this.ctx.fillStyle = `rgb(${color.r}, ${color.g}, ${color.b})`;
      this.ctx.beginPath();
      this.ctx.arc(x, y, ledSize * 0.6, 0, Math.PI * 2);
      this.ctx.fill();
    }
  }

  renderCircle() {
    const centerX = this.canvas.width / 2;
    const centerY = this.canvas.height / 2;
    const radius = Math.min(centerX, centerY) * 0.8;
    const ledSize = Math.min((2 * Math.PI * radius) / this.currentLEDCount * 0.8, 12);

    for (let i = 0; i < this.currentLEDCount; i++) {
      const ledIndex = this.mapLEDIndex(i, this.currentLEDCount, this.leds.length);
      const color = this.leds[ledIndex];

      const angle = (i / this.currentLEDCount) * Math.PI * 2 - Math.PI / 2;
      const x = centerX + Math.cos(angle) * radius;
      const y = centerY + Math.sin(angle) * radius;

      // Draw LED glow
      const gradient = this.ctx.createRadialGradient(x, y, 0, x, y, ledSize);
      gradient.addColorStop(0, `rgb(${color.r}, ${color.g}, ${color.b})`);
      gradient.addColorStop(0.5, `rgba(${color.r}, ${color.g}, ${color.b}, 0.5)`);
      gradient.addColorStop(1, 'rgba(0, 0, 0, 0)');

      this.ctx.fillStyle = gradient;
      this.ctx.beginPath();
      this.ctx.arc(x, y, ledSize, 0, Math.PI * 2);
      this.ctx.fill();

      // Draw LED core
      this.ctx.fillStyle = `rgb(${color.r}, ${color.g}, ${color.b})`;
      this.ctx.beginPath();
      this.ctx.arc(x, y, ledSize * 0.6, 0, Math.PI * 2);
      this.ctx.fill();
    }
  }

  mapLEDIndex(previewIndex, previewCount, realCount) {
    // Equidistant sampling: map preview LED to real LED
    if (realCount <= previewCount) {
      // Fewer real LEDs than preview - stretch
      return Math.floor((previewIndex / previewCount) * realCount);
    } else {
      // More real LEDs than preview - sample
      return Math.floor((previewIndex / previewCount) * realCount);
    }
  }

  updateInfo() {
    const fpsElement = document.getElementById('peekFPS');
    const ledCountElement = document.getElementById('peekLEDCount');
    const effectElement = document.getElementById('peekEffect');

    if (fpsElement) fpsElement.textContent = `${this.fps} FPS`;
    if (ledCountElement) ledCountElement.textContent = `${this.currentLEDCount} LEDs`;
    if (effectElement) effectElement.textContent = this.effectName;
  }

  start() {
    if (this.isRunning) return;
    this.isRunning = true;
    this.lastFrameTime = performance.now();
    this.animate();
  }

  stop() {
    this.isRunning = false;
    if (this.animationFrameId) {
      cancelAnimationFrame(this.animationFrameId);
      this.animationFrameId = null;
    }
  }

  animate(currentTime = performance.now()) {
    if (!this.isRunning) return;

    // Calculate FPS
    const deltaTime = currentTime - this.lastFrameTime;
    this.frameCount++;

    if (currentTime - this.lastFPSUpdate >= this.fpsUpdateInterval) {
      this.fps = Math.round((this.frameCount * 1000) / (currentTime - this.lastFPSUpdate));
      this.frameCount = 0;
      this.lastFPSUpdate = currentTime;
      this.updateInfo();
    }

    this.lastFrameTime = currentTime;

    // Render frame
    this.render();

    // Schedule next frame
    this.animationFrameId = requestAnimationFrame((t) => this.animate(t));
  }
}

// Initialize when DOM is ready
if (typeof window !== 'undefined') {
  window.addEventListener('DOMContentLoaded', () => {
    // Create global instance
    window.ledPreview = new LEDPreview();
  });
}
