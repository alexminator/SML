/**
 * EffectsHandler - LED effects configuration and management
 *
 * Handles effect activation, parameter configuration, and UI deployment
 * for desktop side panels and mobile bottom sheets.
 */

// Initialize logger
const effectsLogger = createLogger('Effects');

class EffectsHandler {
  constructor() {
    // Effect definitions with parameters
    this.effects = {
      'Fire': {
        id: 'Fire',
        name: 'Fire',
        buttonId: 'Firebutton',
        parameters: [
          { name: 'speed', label: 'Speed', type: 'range', min: 1, max: 100, default: 50, unit: '%' },
          { name: 'intensity', label: 'Intensity', type: 'range', min: 1, max: 255, default: 120, unit: '' },
          { name: 'cooling', label: 'Cooling', type: 'range', min: 0, max  : 100, default: 50, unit: '' }
        ]
      },
      'MovingDot': {
        id: 'MovingDot',
        name: 'Dots',
        buttonId: 'MovingDotbutton',
        parameters: [
          { name: 'speed', label: 'Speed', type: 'range', min: 1, max: 100, default: 50, unit: '%' },
          { name: 'trail', label: 'Trail Length', type: 'range', min: 1, max: 50, default: 10, unit: 'px' },
          { name: 'color', label: 'Base Color', type: 'color', default: '#ff6600' }
        ]
      },
      'RainbowBeat': {
        id: 'RainbowBeat',
        name: 'Rainbow',
        buttonId: 'RainbowBeatbutton',
        parameters: [
          { name: 'speed', label: 'Speed', type: 'range', min: 1, max: 100, default: 60, unit: '%' },
          { name: 'hue', label: 'Hue Range', type: 'range', min: 0, max: 255, default: 255, unit: '' },
          { name: 'fade', label: 'Fade Amount', type: 'range', min: 1, max: 255, default: 64, unit: '' }
        ]
      },
      'RWB': {
        id: 'RWB',
        name: 'RWB',
        buttonId: 'RWBbutton',
        parameters: [
          { name: 'speed', label: 'Speed', type: 'range', min: 1, max: 100, default: 40, unit: '%' }
        ]
      },
      'Ripple': {
        id: 'Ripple',
        name: 'Ripple',
        buttonId: 'Ripplebutton',
        parameters: [
          { name: 'speed', label: 'Speed', type: 'range', min: 1, max: 100, default: 50, unit: '%' },
          { name: 'color', label: 'Base Color', type: 'color', default: '#00d4ff' },
          { name: 'fade', label: 'Fade Rate', type: 'range', min: 1, max: 50, default: 10, unit: '' }
        ]
      },
      'Twinkle': {
        id: 'Twinkle',
        name: 'Twinkle',
        buttonId: 'Twinklebutton',
        parameters: [
          { name: 'speed', label: 'Speed', type: 'range', min: 1, max: 100, default: 55, unit: '%' },
          { name: 'density', label: 'Density', type: 'range', min: 1, max: 100, default: 50, unit: '%' },
          { name: 'color', label: 'Color', type: 'select', options: ['Random', 'Red', 'Green', 'Blue', 'White'], default: 'Random' }
        ]
      },
      'Balls': {
        id: 'Balls',
        name: 'Balls',
        buttonId: 'Ballsbutton',
        parameters: [
          { name: 'count', label: 'Ball Count', type: 'range', min: 1, max: 16, default: 4, unit: '' },
          { name: 'gravity', label: 'Gravity', type: 'range', min: 1, max: 100, default: 50, unit: '%' },
          { name: 'bounce', label: 'Bounciness', type: 'range', min: 1, max: 100, default: 80, unit: '%' }
        ]
      },
      'Juggle': {
        id: 'Juggle',
        name: 'Juggle',
        buttonId: 'Jugglebutton',
        parameters: [
          { name: 'speed', label: 'Speed', type: 'range', min: 1, max: 100, default: 50, unit: '%' },
          { name: 'fade', label: 'Fade Amount', type: 'range', min: 1, max: 50, default: 8, unit: '' }
        ]
      },
      'Sinelon': {
        id: 'Sinelon',
        name: 'Sinelon',
        buttonId: 'Sinelonbutton',
        parameters: [
          { name: 'speed', label: 'Speed', type: 'range', min: 1, max: 100, default: 50, unit: '%' },
          { name: 'fade', label: 'Fade Amount', type: 'range', min: 1, max: 50, default: 10, unit: '' },
          { name: 'hue', label: 'Hue Speed', type: 'range', min: 1, max: 100, default: 30, unit: '%' }
        ]
      },
      'Comet': {
        id: 'Comet',
        name: 'Comet',
        buttonId: 'Cometbutton',
        parameters: [
          { name: 'speed', label: 'Speed', type: 'range', min: 1, max: 100, default: 50, unit: '%' },
          { name: 'trail', label: 'Trail Length', type: 'range', min: 1, max: 100, default: 70, unit: '%' },
          { name: 'color', label: 'Comet Color', type: 'color', default: '#ff0066' }
        ]
      },
      'Breath': {
        id: 'Breath',
        name: 'Breath',
        buttonId: 'Breathbutton',
        parameters: [
          { name: 'speed', label: 'Breath Speed', type: 'range', min: 1, max: 100, default: 30, unit: '%' },
          { name: 'minBrightness', label: 'Min Brightness', type: 'range', min: 0, max: 50, default: 10, unit: '%' },
          { name: 'maxBrightness', label: 'Max Brightness', type: 'range', min: 50, max: 100, default: 100, unit: '%' },
          { name: 'color', label: 'Color', type: 'color', default: '#00d4ff' }
        ]
      },
      'ColorSweep': {
        id: 'ColorSweep',
        name: 'ColorSweep',
        buttonId: 'ColorSweepbutton',
        parameters: [
          { name: 'speed', label: 'Speed', type: 'range', min: 1, max: 100, default: 40, unit: '%' },
          { name: 'hueRange', label: 'Hue Range', type: 'range', min: 1, max: 255, default: 100, unit: '' },
          { name: 'direction', label: 'Direction', type: 'select', options: ['Forward', 'Backward', 'Ping Pong'], default: 'Forward' }
        ]
      }
    };

    this.currentEffect = null;
    this.panelOpen = false;
    this.isMobile = window.innerWidth <= 767;
    this.longPressTimer = null;
    this.longPressDuration = 500;

    this.init();
  }

  /**
   * Initialize the effects handler
   */
  init() {
    this.setupEventListeners();
    this.setupConfigButtons();
    this.setupMobileTouchHandlers();

    // Listen for resize events
    window.addEventListener('resize', () => {
      this.isMobile = window.innerWidth <= 767;
    });
  }

  /**
   * Setup event listeners for effect buttons
   */
  setupEventListeners() {
    // Add click handlers to all effect buttons
    Object.keys(this.effects).forEach(effectKey => {
      const effect = this.effects[effectKey];
      const button = document.getElementById(effect.buttonId);

      if (button) {
        button.addEventListener('click', (e) => {
          // If long press was triggered, don't toggle effect
          if (button.hasAttribute('data-long-press')) {
            button.removeAttribute('data-long-press');
            return;
          }
          this.toggleEffect(effectKey);
        });
      }
    });

    // Listen for tab changes to close panel
    document.addEventListener('tabChanged', () => {
      this.closePanel();
    });
  }

  /**
   * Setup configuration buttons for desktop
   */
  setupConfigButtons() {
    Object.keys(this.effects).forEach(effectKey => {
      const effect = this.effects[effectKey];
      const button = document.getElementById(effect.buttonId);

      if (button) {
        // Check if config button already exists
        let configBtn = button.parentElement.querySelector('.sml-effect-config-btn');

        if (!configBtn && !this.isMobile) {
          // Create config button
          configBtn = document.createElement('button');
          configBtn.className = 'sml-effect-config-btn';
          configBtn.innerHTML = '<span class="fas fa-cog"></span>';
          configBtn.setAttribute('aria-label', `Configure ${effect.name} effect`);
          configBtn.setAttribute('data-effect', effectKey);

          // Position relative to button parent
          const parent = button.parentElement;
          parent.style.position = 'relative';
          parent.appendChild(configBtn);

          // Add click handler
          configBtn.addEventListener('click', (e) => {
            e.stopPropagation();
            this.openConfigPanel(effectKey);
          });
        }
      }
    });
  }

  /**
   * Setup mobile touch handlers for long-press
   */
  setupMobileTouchHandlers() {
    Object.keys(this.effects).forEach(effectKey => {
      const effect = this.effects[effectKey];
      const button = document.getElementById(effect.buttonId);

      if (button) {
        let pressTimer;

        button.addEventListener('touchstart', (e) => {
          if (!this.isMobile) return;

          pressTimer = setTimeout(() => {
            button.setAttribute('data-long-press', 'true');
            this.openConfigPanel(effectKey);

            // Haptic feedback if available
            if (navigator.vibrate) {
              navigator.vibrate(50);
            }
          }, this.longPressDuration);
        });

        button.addEventListener('touchend', () => {
          clearTimeout(pressTimer);
        });

        button.addEventListener('touchmove', () => {
          clearTimeout(pressTimer);
        });

        button.addEventListener('touchcancel', () => {
          clearTimeout(pressTimer);
        });
      }
    });
  }

  /**
   * Toggle an effect on/off
   * @param {string} effectKey - The effect key
   */
  toggleEffect(effectKey) {
    const effect = this.effects[effectKey];
    const button = document.getElementById(effect.buttonId);

    if (!button) return;

    const isActive = button.classList.contains('active');

    if (isActive) {
      // Turn off effect
      button.classList.remove('active');
      this.sendEffectCommand(effectKey, false);
    } else {
      // Turn on effect
      // First, turn off all other effects
      Object.keys(this.effects).forEach(key => {
        const otherEffect = this.effects[key];
        const otherButton = document.getElementById(otherEffect.buttonId);
        if (otherButton && otherButton.classList.contains('active')) {
          otherButton.classList.remove('active');
          this.sendEffectCommand(key, false);
        }
      });

      // Turn on selected effect
      button.classList.add('active');
      this.currentEffect = effectKey;
      this.sendEffectCommand(effectKey, true);
    }
  }

  /**
   * Send effect command to ESP32
   * @param {string} effectKey - The effect key
   * @param {boolean} state - The effect state (true=on, false=off)
   */
  sendEffectCommand(effectKey, state) {
    const effect = this.effects[effectKey];
    const command = {
      type: 'effect',
      effect: effect.id,
      state: state ? 'on' : 'off',
      parameters: {}
    };

    // Send via WebSocket if connected
    if (window.websocket && window.websocket.readyState === WebSocket.OPEN) {
      window.websocket.send(JSON.stringify(command));
    } else {
      // Fallback to HTTP POST
      fetch('/api/effect', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(command)
      }).catch(err => effectsLogger.error('Effect command failed:', err));
    }
  }

  /**
   * Open configuration panel for an effect
   * @param {string} effectKey - The effect key
   */
  openConfigPanel(effectKey) {
    const effect = this.effects[effectKey];
    this.currentEffect = effectKey;

    if (this.isMobile) {
      this.openMobileSheet(effect);
    } else {
      this.openDesktopPanel(effect);
    }
  }

  /**
   * Open desktop side panel
   * @param {Object} effect - The effect object
   */
  openDesktopPanel(effect) {
    let panel = document.getElementById('effect-config-panel');

    if (!panel) {
      panel = this.createDesktopPanel();
      document.body.appendChild(panel);
    }

    // Update panel content
    this.updatePanelContent(panel, effect);

    // Open panel
    panel.classList.add('open');
    this.panelOpen = true;

    // Setup close handlers
    this.setupPanelCloseHandlers(panel);
  }

  /**
   * Open mobile bottom sheet
   * @param {Object} effect - The effect object
   */
  openMobileSheet(effect) {
    let sheet = document.getElementById('effect-config-sheet');

    if (!sheet) {
      sheet = this.createMobileSheet();
      document.body.appendChild(sheet);
    }

    // Update sheet content
    this.updateSheetContent(sheet, effect);

    // Open sheet
    sheet.classList.add('open');
    this.panelOpen = true;

    // Setup close handlers
    this.setupSheetCloseHandlers(sheet);
  }

  /**
   * Create desktop side panel element
   */
  createDesktopPanel() {
    const panel = document.createElement('div');
    panel.id = 'effect-config-panel';
    panel.className = 'sml-config-panel';

    panel.innerHTML = `
      <div class="sml-config-panel-header">
        <h3 id="panel-effect-name">Effect Config</h3>
        <button class="sml-close-panel" aria-label="Close panel">
          <span class="fas fa-times"></span>
        </button>
      </div>
      <div class="sml-config-panel-body" id="panel-body">
        <!-- Dynamic content will be inserted here -->
      </div>
    `;

    return panel;
  }

  /**
   * Create mobile bottom sheet element
   */
  createMobileSheet() {
    const sheet = document.createElement('div');
    sheet.id = 'effect-config-sheet';
    sheet.className = 'sml-bottom-sheet';

    sheet.innerHTML = `
      <div class="sml-bottom-sheet-header">
        <h3 id="sheet-effect-name">Effect Config</h3>
        <button class="sml-close-sheet" aria-label="Close sheet">
          <span class="fas fa-times"></span>
        </button>
      </div>
      <div class="sml-bottom-sheet-body" id="sheet-body">
        <!-- Dynamic content will be inserted here -->
      </div>
    `;

    return sheet;
  }

  /**
   * Update desktop panel content with effect parameters
   * @param {HTMLElement} panel - The panel element
   * @param {Object} effect - The effect object
   */
  updatePanelContent(panel, effect) {
    const title = panel.querySelector('#panel-effect-name');
    const body = panel.querySelector('#panel-body');

    title.textContent = effect.name;

    let html = '';

    if (effect.parameters && effect.parameters.length > 0) {
      effect.parameters.forEach(param => {
        html += this.createParameterInput(param);
      });

      html += `
        <div class="sml-effect-actions">
          <button class="sml-btn-apply" id="apply-params">
            <span class="fas fa-check"></span> Apply
          </button>
          <button class="sml-btn-reset" id="reset-params">
            <span class="fas fa-undo"></span> Reset
          </button>
        </div>
      `;
    } else {
      html = '<p class="sml-no-params">This effect has no configurable parameters.</p>';
    }

    body.innerHTML = html;

    // Setup parameter input handlers
    this.setupParameterHandlers(effect, body);
  }

  /**
   * Update mobile sheet content with effect parameters
   * @param {HTMLElement} sheet - The sheet element
   * @param {Object} effect - The effect object
   */
  updateSheetContent(sheet, effect) {
    const title = sheet.querySelector('#sheet-effect-name');
    const body = sheet.querySelector('#sheet-body');

    title.textContent = effect.name;

    let html = '';

    if (effect.parameters && effect.parameters.length > 0) {
      effect.parameters.forEach(param => {
        html += this.createParameterInput(param);
      });

      html += `
        <div class="sml-effect-actions">
          <button class="sml-btn-apply" id="apply-params">
            <span class="fas fa-check"></span> Apply
          </button>
          <button class="sml-btn-reset" id="reset-params">
            <span class="fas fa-undo"></span> Reset
          </button>
        </div>
      `;
    } else {
      html = '<p class="sml-no-params">This effect has no configurable parameters.</p>';
    }

    body.innerHTML = html;

    // Setup parameter input handlers
    this.setupParameterHandlers(effect, body);
  }

  /**
   * Create parameter input HTML
   * @param {Object} param - The parameter object
   */
  createParameterInput(param) {
    let inputHtml = '';

    switch (param.type) {
      case 'range':
        inputHtml = `
          <div class="sml-effect-param-group">
            <label for="param-${param.name}">${param.label}: <span id="value-${param.name}">${param.default}</span>${param.unit}</label>
            <input type="range" id="param-${param.name}" name="${param.name}"
                   min="${param.min}" max="${param.max}" value="${param.default}"
                   data-default="${param.default}">
          </div>
        `;
        break;

      case 'color':
        inputHtml = `
          <div class="sml-effect-param-group">
            <label for="param-${param.name}">${param.label}</label>
            <div class="sml-color-input-wrapper">
              <input type="color" id="param-${param.name}" name="${param.name}"
                     value="${param.default}" data-default="${param.default}">
              <span class="sml-color-value">${param.default}</span>
            </div>
          </div>
        `;
        break;

      case 'select':
        const options = param.options.map(opt =>
          `<option value="${opt}" ${opt === param.default ? 'selected' : ''}>${opt}</option>`
        ).join('');

        inputHtml = `
          <div class="sml-effect-param-group">
            <label for="param-${param.name}">${param.label}</label>
            <select id="param-${param.name}" name="${param.name}" data-default="${param.default}">
              ${options}
            </select>
          </div>
        `;
        break;
    }

    return inputHtml;
  }

  /**
   * Setup parameter input handlers
   * @param {Object} effect - The effect object
   * @param {HTMLElement} container - The container element
   */
  setupParameterHandlers(effect, container) {
    // Range inputs - update value display
    container.querySelectorAll('input[type="range"]').forEach(input => {
      input.addEventListener('input', (e) => {
        const valueSpan = document.getElementById(`value-${e.target.name}`);
        if (valueSpan) {
          valueSpan.textContent = e.target.value;
        }
      });
    });

    // Color inputs - update hex display
    container.querySelectorAll('input[type="color"]').forEach(input => {
      input.addEventListener('input', (e) => {
        const valueSpan = e.target.parentElement.querySelector('.sml-color-value');
        if (valueSpan) {
          valueSpan.textContent = e.target.value;
        }
      });
    });

    // Apply button
    const applyBtn = container.querySelector('#apply-params');
    if (applyBtn) {
      applyBtn.addEventListener('click', () => {
        this.applyParameters(effect);
      });
    }

    // Reset button
    const resetBtn = container.querySelector('#reset-params');
    if (resetBtn) {
      resetBtn.addEventListener('click', () => {
        this.resetParameters(effect, container);
      });
    }
  }

  /**
   * Apply effect parameters
   * @param {Object} effect - The effect object
   */
  applyParameters(effect) {
    const container = this.isMobile
      ? document.querySelector('.sml-bottom-sheet-body')
      : document.querySelector('.sml-config-panel-body');

    if (!container) return;

    const parameters = {};

    effect.parameters.forEach(param => {
      const input = container.querySelector(`#param-${param.name}`);
      if (input) {
        parameters[param.name] = input.value;
      }
    });

    const command = {
      type: 'effect',
      effect: effect.id,
      action: 'configure',
      parameters: parameters
    };

    // Send via WebSocket if connected
    if (window.websocket && window.websocket.readyState === WebSocket.OPEN) {
      window.websocket.send(JSON.stringify(command));
    } else {
      // Fallback to HTTP POST
      fetch('/api/effect/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(command)
      }).then(() => {
        this.showApplySuccess();
      }).catch(err => {
        effectsLogger.error('Failed to apply parameters:', err);
        this.showApplyError();
      });
    }

    this.closePanel();
  }

  /**
   * Reset effect parameters to defaults
   * @param {Object} effect - The effect object
   * @param {HTMLElement} container - The container element
   */
  resetParameters(effect, container) {
    effect.parameters.forEach(param => {
      const input = container.querySelector(`#param-${param.name}`);
      if (input) {
        input.value = param.default;

        // Update display
        if (param.type === 'range') {
          const valueSpan = document.getElementById(`value-${param.name}`);
          if (valueSpan) {
            valueSpan.textContent = param.default;
          }
        } else if (param.type === 'color') {
          const valueSpan = input.parentElement.querySelector('.sml-color-value');
          if (valueSpan) {
            valueSpan.textContent = param.default;
          }
        }
      }
    });
  }

  /**
   * Setup panel close handlers
   * @param {HTMLElement} panel - The panel element
   */
  setupPanelCloseHandlers(panel) {
    const closeBtn = panel.querySelector('.sml-close-panel');

    if (closeBtn) {
      closeBtn.addEventListener('click', () => {
        this.closePanel();
      });
    }

    // Click outside to close
    panel.addEventListener('click', (e) => {
      if (e.target === panel) {
        this.closePanel();
      }
    });
  }

  /**
   * Setup sheet close handlers
   * @param {HTMLElement} sheet - The sheet element
   */
  setupSheetCloseHandlers(sheet) {
    const closeBtn = sheet.querySelector('.sml-close-sheet');

    if (closeBtn) {
      closeBtn.addEventListener('click', () => {
        this.closePanel();
      });
    }

    // Swipe down to close
    let startY = 0;
    sheet.addEventListener('touchstart', (e) => {
      startY = e.touches[0].clientY;
    });

    sheet.addEventListener('touchmove', (e) => {
      const currentY = e.touches[0].clientY;
      const diff = currentY - startY;

      if (diff > 50 && sheet.scrollTop === 0) {
        this.closePanel();
      }
    });
  }

  /**
   * Close the configuration panel/sheet
   */
  closePanel() {
    const panel = document.getElementById('effect-config-panel');
    const sheet = document.getElementById('effect-config-sheet');

    if (panel) {
      panel.classList.remove('open');
    }

    if (sheet) {
      sheet.classList.remove('open');
    }

    this.panelOpen = false;
  }

  /**
   * Show apply success notification
   */
  showApplySuccess() {
    this.showNotification('Effect parameters applied successfully', 'success');
  }

  /**
   * Show apply error notification
   */
  showApplyError() {
    this.showNotification('Failed to apply effect parameters', 'error');
  }

  /**
   * Show notification message
   * @param {string} message - The message to display
   * @param {string} type - The notification type (success/error)
   */
  showNotification(message, type = 'success') {
    // Create notification element
    const notification = document.createElement('div');
    notification.className = `sml-notification sml-notification-${type}`;
    notification.innerHTML = `
      <span class="fas fa-${type === 'success' ? 'check-circle' : 'exclamation-circle'}"></span>
      <span>${message}</span>
    `;

    document.body.appendChild(notification);

    // Show notification
    setTimeout(() => notification.classList.add('show'), 10);

    // Auto-hide after 3 seconds
    setTimeout(() => {
      notification.classList.remove('show');
      setTimeout(() => notification.remove(), 300);
    }, 3000);
  }

  /**
   * Get current effect
   * @returns {string|null} The current effect key
   */
  getCurrentEffect() {
    return this.currentEffect;
  }

  /**
   * Check if panel is open
   * @returns {boolean} Whether the panel is open
   */
  isPanelOpen() {
    return this.panelOpen;
  }
}

// Create global instance
const effectsHandler = new EffectsHandler();

// Initialize on DOM content loaded
document.addEventListener('DOMContentLoaded', () => {
  // Wait for tabs manager to be ready
  setTimeout(() => {
    effectsHandler.init();
  }, 100);
});

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
  module.exports = { EffectsHandler, effectsHandler };
}
