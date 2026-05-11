/**
 * ConfigManager - WiFi and system configuration management
 *
 * Handles WiFi configuration forms, password validation, LED settings,
 * help system, system info updates, and restart countdown.
 */

class ConfigManager {
  constructor() {
    this.wifiForm = null;
    this.ledForm = null;
    this.restartTimer = null;
    this.restartCount = 10;
    this.systemInfo = {};
    this.scanningNetworks = false;

    this.init();
  }

  /**
   * Initialize the config manager
   */
  init() {
    this.setupWiFiForm();
    this.setupLEDConfig();
    this.setupHelpSystem();
    this.setupSystemInfo();
    this.loadCurrentConfig();
    this.setupKeyboardShortcuts();
  }

  /**
   * Setup WiFi configuration form
   */
  setupWiFiForm() {
    this.wifiForm = document.getElementById('wifiConfigForm');

    if (!this.wifiForm) {
      console.warn('WiFi configuration form not found');
      return;
    }

    const passwordInput = document.getElementById('newPassword');
    const ssidInput = document.getElementById('newSSID');

    // Password strength validation
    if (passwordInput) {
      passwordInput.addEventListener('input', (e) => {
        this.validatePasswordStrength(e.target.value);
      });
    }

    // Form submission
    this.wifiForm.addEventListener('submit', (e) => {
      e.preventDefault();
      this.handleWiFiConfigSubmit();
    });

    // Network scan button (future feature)
    const scanBtn = document.getElementById('scanNetworks');
    if (scanBtn) {
      scanBtn.addEventListener('click', () => {
        this.scanNetworks();
      });
    }
  }

  /**
   * Validate password strength
   * @param {string} password - The password to validate
   */
  validatePasswordStrength(password) {
    const strengthBar = document.querySelector('.strength-bar-fill');
    const strengthText = document.querySelector('.strength-text');

    if (!strengthBar || !strengthText) return;

    let strength = 0;

    // Length check
    if (password.length >= 8) strength += 1;
    if (password.length >= 12) strength += 1;

    // Complexity checks
    if (/[a-z]/.test(password)) strength += 1;
    if (/[A-Z]/.test(password)) strength += 1;
    if (/[0-9]/.test(password)) strength += 1;
    if (/[^a-zA-Z0-9]/.test(password)) strength += 1;

    // Update UI
    strengthBar.className = 'strength-bar-fill';

    if (strength <= 2) {
      strengthBar.classList.add('weak');
      strengthText.textContent = 'Weak password';
    } else if (strength <= 4) {
      strengthBar.classList.add('medium');
      strengthText.textContent = 'Medium strength';
    } else {
      strengthBar.classList.add('strong');
      strengthText.textContent = 'Strong password';
    }
  }

  /**
   * Handle WiFi configuration form submission
   */
  async handleWiFiConfigSubmit() {
    const ssid = document.getElementById('newSSID').value.trim();
    const password = document.getElementById('newPassword').value;

    // Validate password
    if (password.length < 8) {
      this.showStatus('error', 'Password must be at least 8 characters long');
      return;
    }

    // Show loading state
    const saveBtn = document.getElementById('wifiSaveBtn');
    if (saveBtn) {
      saveBtn.classList.add('loading');
      saveBtn.disabled = true;
    }

    // Disable form
    this.setFormState(this.wifiForm, 'disabled');

    try {
      // Send configuration to ESP32
      const response = await fetch('/api/wifi/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          ssid: ssid || null, // Keep current if empty
          password: password
        })
      });

      if (!response.ok) {
        throw new Error('Failed to configure WiFi');
      }

      const result = await response.json();

      if (result.success) {
        this.showStatus('success', 'WiFi configuration saved successfully');
        this.startRestartCountdown();
      } else {
        throw new Error(result.message || 'Configuration failed');
      }

    } catch (error) {
      console.error('WiFi config error:', error);
      this.showStatus('error', error.message || 'Failed to configure WiFi. Please try again.');
      this.setFormState(this.wifiForm, 'enabled');

      // Reset button state
      if (saveBtn) {
        saveBtn.classList.remove('loading');
        saveBtn.disabled = false;
      }
    }
  }

  /**
   * Setup LED hardware configuration
   */
  setupLEDConfig() {
    const ledConfigForm = document.getElementById('ledConfigForm');

    if (!ledConfigForm) return;

    ledConfigForm.addEventListener('submit', async (e) => {
      e.preventDefault();
      await this.handleLEDConfigSubmit();
    });

    // Add input validation for numeric fields
    const numericInputs = ledConfigForm.querySelectorAll('input[type="number"]');
    numericInputs.forEach(input => {
      input.addEventListener('input', (e) => {
        const min = parseFloat(e.target.min);
        const max = parseFloat(e.target.max);
        const value = parseFloat(e.target.value);

        if (value < min) e.target.value = min;
        if (value > max) e.target.value = max;
      });
    });
  }

  /**
   * Handle LED configuration form submission
   */
  async handleLEDConfigSubmit() {
    const form = document.getElementById('ledConfigForm');
    const formData = new FormData(form);

    const config = {
      numLeds: parseInt(formData.get('numLeds')),
      brightness: parseInt(formData.get('brightness')),
      maxMilliamps: parseInt(formData.get('maxMilliamps')),
      colorOrder: formData.get('colorOrder')
    };

    // Show loading state
    const saveBtn = document.getElementById('ledSaveBtn');
    if (saveBtn) {
      saveBtn.classList.add('loading');
      saveBtn.disabled = true;
    }

    this.setFormState(form, 'disabled');

    try {
      const response = await fetch('/api/led/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(config)
      });

      if (!response.ok) {
        throw new Error('Failed to configure LED');
      }

      const result = await response.json();

      if (result.success) {
        this.showStatus('success', 'LED configuration saved');
      } else {
        throw new Error(result.message || 'Configuration failed');
      }

    } catch (error) {
      console.error('LED config error:', error);
      this.showStatus('error', error.message || 'Failed to configure LED. Please try again.');
    } finally {
      this.setFormState(form, 'enabled');

      // Reset button state
      if (saveBtn) {
        saveBtn.classList.remove('loading');
        saveBtn.disabled = false;
      }
    }
  }

  /**
   * Setup help system
   */
  setupHelpSystem() {
    // Help card expand/collapse
    const helpCards = document.querySelectorAll('.help-card');

    helpCards.forEach(card => {
      const title = card.querySelector('h4');
      const content = card.querySelector('p');

      if (title && content) {
        title.style.cursor = 'pointer';
        title.addEventListener('click', () => {
          content.classList.toggle('expanded');
          card.classList.toggle('expanded');
        });
      }
    });

    // FAQ accordion (if present)
    const faqItems = document.querySelectorAll('.faq-item');

    faqItems.forEach(item => {
      const question = item.querySelector('.faq-question');
      const answer = item.querySelector('.faq-answer');

      if (question && answer) {
        question.addEventListener('click', () => {
          const isOpen = item.classList.contains('open');

          // Close all other FAQs
          faqItems.forEach(otherItem => {
            otherItem.classList.remove('open');
            const otherAnswer = otherItem.querySelector('.faq-answer');
            if (otherAnswer) {
              otherAnswer.style.maxHeight = null;
            }
          });

          // Toggle current FAQ
          if (!isOpen) {
            item.classList.add('open');
            answer.style.maxHeight = answer.scrollHeight + 'px';
          }
        });
      }
    });
  }

  /**
   * Setup system info updates via WebSocket
   */
  setupSystemInfo() {
    // Update via WebSocket when available
    document.addEventListener('websocketMessage', (e) => {
      const data = e.detail;

      if (data.type === 'systemInfo') {
        this.updateSystemInfo(data);
      }
    });

    // Initial system info fetch
    this.fetchSystemInfo();

    // Refresh system info every 30 seconds
    setInterval(() => {
      this.fetchSystemInfo();
    }, 30000);
  }

  /**
   * Fetch system information from ESP32
   */
  async fetchSystemInfo() {
    try {
      const response = await fetch('/api/system/info');

      if (!response.ok) {
        throw new Error('Failed to fetch system info');
      }

      const data = await response.json();
      this.updateSystemInfo(data);

    } catch (error) {
      console.error('System info fetch error:', error);
    }
  }

  /**
   * Update system info display
   * @param {Object} data - System info data
   */
  updateSystemInfo(data) {
    this.systemInfo = data;

    // Update firmware version
    const versionEl = document.getElementById('firmware-version');
    if (versionEl && data.version) {
      versionEl.textContent = data.version;
    }

    // Update uptime
    const uptimeEl = document.getElementById('uptime');
    if (uptimeEl && data.uptime) {
      uptimeEl.textContent = this.formatUptime(data.uptime);
    }

    // Update free heap
    const heapEl = document.getElementById('free-heap');
    if (heapEl && data.freeHeap) {
      heapEl.textContent = this.formatBytes(data.freeHeap);
    }

    // Update WiFi info
    const ssidEl = document.getElementById('currentSSID');
    if (ssidEl && data.ssid) {
      ssidEl.textContent = data.ssid;
    }

    const signalEl = document.getElementById('currentSignal');
    if (signalEl && data.rssi) {
      signalEl.textContent = this.formatSignal(data.rssi);
    }

    const ipEl = document.getElementById('currentIP');
    if (ipEl && data.ip) {
      ipEl.textContent = data.ip;
    }
  }

  /**
   * Load current configuration
   */
  async loadCurrentConfig() {
    try {
      const response = await fetch('/api/config');

      if (!response.ok) {
        throw new Error('Failed to load configuration');
      }

      const config = await response.json();

      // Populate WiFi SSID (password not returned for security)
      if (config.wifi && config.wifi.ssid) {
        const ssidInput = document.getElementById('newSSID');
        if (ssidInput) {
          ssidInput.placeholder = `Current: ${config.wifi.ssid}`;
        }
      }

      // Populate LED config
      if (config.led) {
        this.populateLEDConfig(config.led);
      }

    } catch (error) {
      console.error('Config load error:', error);
    }
  }

  /**
   * Populate LED configuration form
   * @param {Object} ledConfig - LED configuration
   */
  populateLEDConfig(ledConfig) {
    const form = document.getElementById('ledConfigForm');

    if (!form) return;

    Object.keys(ledConfig).forEach(key => {
      const input = form.querySelector(`[name="${key}"]`);

      if (input) {
        input.value = ledConfig[key];
      }
    });
  }

  /**
   * Start restart countdown
   */
  startRestartCountdown() {
    const countdownEl = document.querySelector('.restart-countdown');

    if (!countdownEl) {
      console.warn('Restart countdown element not found');
      return;
    }

    this.restartCount = 10;
    countdownEl.classList.add('active');

    const numberEl = countdownEl.querySelector('.countdown-number');
    const cancelBtn = countdownEl.querySelector('.cancel-btn');

    // Update countdown
    const updateCountdown = () => {
      if (numberEl) {
        numberEl.textContent = this.restartCount;
      }
    };

    updateCountdown();

    // Setup cancel button
    if (cancelBtn) {
      cancelBtn.onclick = () => {
        this.cancelRestart();
      };
    }

    // Start countdown timer
    this.restartTimer = setInterval(() => {
      this.restartCount--;
      updateCountdown();

      if (this.restartCount <= 0) {
        clearInterval(this.restartTimer);
        // ESP32 will restart, show message
        countdownEl.innerHTML = `
          <h3><span class="fas fa-check-circle"></span> Restarting...</h3>
          <p>The device is restarting. Please reconnect in a few seconds.</p>
        `;
      }
    }, 1000);
  }

  /**
   * Cancel restart countdown
   */
  cancelRestart() {
    if (this.restartTimer) {
      clearInterval(this.restartTimer);
      this.restartTimer = null;
    }

    const countdownEl = document.querySelector('.restart-countdown');
    if (countdownEl) {
      countdownEl.classList.remove('active');
    }

    this.setFormState(this.wifiForm, 'enabled');
  }

  /**
   * Scan for available WiFi networks (future feature)
   */
  async scanNetworks() {
    if (this.scanningNetworks) return;

    this.scanningNetworks = true;
    const scanBtn = document.getElementById('scanNetworks');

    if (scanBtn) {
      scanBtn.disabled = true;
      scanBtn.innerHTML = '<span class="sml-loading-spinner"></span> Scanning...';
    }

    try {
      const response = await fetch('/api/wifi/scan');

      if (!response.ok) {
        throw new Error('Failed to scan networks');
      }

      const networks = await response.json();
      this.displayNetworks(networks);

    } catch (error) {
      console.error('Network scan error:', error);
      this.showStatus('error', 'Failed to scan for networks');
    } finally {
      this.scanningNetworks = false;

      if (scanBtn) {
        scanBtn.disabled = false;
        scanBtn.innerHTML = '<span class="fas fa-search"></span> Scan Networks';
      }
    }
  }

  /**
   * Display scanned networks
   * @param {Array} networks - Array of network objects
   */
  displayNetworks(networks) {
    const listEl = document.querySelector('.network-list');

    if (!listEl) return;

    listEl.innerHTML = '';

    networks.forEach(network => {
      const item = document.createElement('div');
      item.className = 'network-item';
      item.dataset.ssid = network.ssid;

      const signalIcon = this.getSignalIcon(network.rssi);
      const lockIcon = network.secured ? '<span class="fas fa-lock network-locked"></span>' : '';

      item.innerHTML = `
        <span class="network-ssid">${network.ssid}</span>
        <div class="network-signal">
          <span class="fas fa-wifi"></span>
          ${this.formatSignal(network.rssi)}
          ${lockIcon}
        </div>
      `;

      item.addEventListener('click', () => {
        const ssidInput = document.getElementById('newSSID');
        if (ssidInput) {
          ssidInput.value = network.ssid;
        }
      });

      listEl.appendChild(item);
    });
  }

  /**
   * Get signal icon based on RSSI
   * @param {number} rssi - Signal strength in dBm
   */
  getSignalIcon(rssi) {
    if (rssi >= -50) return 'fa-signal';
    if (rssi >= -60) return 'fa-signal (good)';
    if (rssi >= -70) return 'fa-signal (fair)';
    return 'fa-signal (weak)';
  }

  /**
   * Setup keyboard shortcuts
   */
  setupKeyboardShortcuts() {
    document.addEventListener('keydown', (e) => {
      // ESC to close modal/panel
      if (e.key === 'Escape') {
        const wifiModal = document.getElementById('wifiModal');
        if (wifiModal && wifiModal.style.display === 'block') {
          this.closeWiFiModal();
        }
      }

      // Ctrl/Cmd + S to save (only when form is focused)
      if ((e.ctrlKey || e.metaKey) && e.key === 's') {
        const activeElement = document.activeElement;

        if (activeElement && this.wifiForm && this.wifiForm.contains(activeElement)) {
          e.preventDefault();
          this.handleWiFiConfigSubmit();
        }
      }
    });
  }

  /**
   * Set form state (enabled/disabled)
   * @param {HTMLFormElement} form - The form element
   * @param {string} state - 'enabled' or 'disabled'
   */
  setFormState(form, state) {
    const inputs = form.querySelectorAll('input, button, select');

    inputs.forEach(input => {
      if (state === 'disabled') {
        input.disabled = true;
      } else {
        input.disabled = false;
      }
    });
  }

  /**
   * Show status message
   * @param {string} type - Message type (success, error, info)
   * @param {string} message - The message to display
   */
  showStatus(type, message) {
    let statusEl = document.querySelector('.config-status');

    if (!statusEl) {
      statusEl = document.createElement('div');
      statusEl.className = 'config-status';
      this.wifiForm.parentElement.appendChild(statusEl);
    }

    statusEl.className = `config-status ${type}`;
    statusEl.innerHTML = `<span class="fas fa-${type === 'success' ? 'check-circle' : type === 'error' ? 'exclamation-circle' : 'info-circle'}"></span> ${message}`;

    // Auto-hide after 5 seconds
    setTimeout(() => {
      statusEl.className = 'config-status';
    }, 5000);
  }

  /**
   * Format uptime in seconds to human-readable string
   * @param {number} seconds - Uptime in seconds
   */
  formatUptime(seconds) {
    const days = Math.floor(seconds / 86400);
    const hours = Math.floor((seconds % 86400) / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);

    const parts = [];

    if (days > 0) parts.push(`${days}d`);
    if (hours > 0) parts.push(`${hours}h`);
    if (minutes > 0) parts.push(`${minutes}m`);

    return parts.join(' ') || '0m';
  }

  /**
   * Format bytes to human-readable string
   * @param {number} bytes - Bytes to format
   */
  formatBytes(bytes) {
    if (bytes < 1024) return `${bytes} B`;
    if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
    return `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
  }

  /**
   * Format WiFi signal strength
   * @param {number} rssi - Signal strength in dBm
   */
  formatSignal(rssi) {
    if (rssi >= -50) return 'Excellent';
    if (rssi >= -60) return 'Good';
    if (rssi >= -70) return 'Fair';
    return 'Weak';
  }
}

// Create global instance
const configManager = new ConfigManager();

// Initialize on DOM content loaded
document.addEventListener('DOMContentLoaded', () => {
  setTimeout(() => {
    configManager.init();
  }, 200);
});

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
  module.exports = { ConfigManager, configManager };
}
