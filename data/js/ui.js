/* ──────────────────────────────────────────────────────────────────────────────
   ui.js — SML UI helpers: Toast, Confirm, Resize, WiFi save, Skeleton helpers
   ────────────────────────────────────────────────────────────────────────────── */

// ============================================================================
// TOAST — FIFO queue, one at a time
// ============================================================================

const _toastQueue = [];
let _toastActive = false;

function showToast(message, type) {
  type = type || 'info';
  _toastQueue.push({ message, type });
  _processToastQueue();
}

function _processToastQueue() {
  if (_toastActive || _toastQueue.length === 0) return;
  _toastActive = true;

  const { message, type } = _toastQueue.shift();
  const container = document.getElementById('toastContainer');
  if (!container) {
    _toastActive = false;
    return;
  }

  const toast = document.createElement('div');
  toast.className = 'toast';
  toast.classList.add(type);

  // Icon map
  const icons = {
    success: '<span class="fas fa-check-circle" style="color:var(--accent-success)"></span>',
    error:   '<span class="fas fa-exclamation-circle" style="color:var(--accent-danger)"></span>',
    warning: '<span class="fas fa-exclamation-triangle" style="color:var(--accent-warning)"></span>',
    info:    '<span class="fas fa-info-circle" style="color:var(--accent-secondary)"></span>',
  };

  toast.innerHTML = `
    <span class="toast-icon">${icons[type] || icons.info}</span>
    <span class="toast-text">${message}</span>
    <span class="toast-progress"></span>
  `;

  container.appendChild(toast);

  // Show next after this one finishes (animation duration + visible time)
  setTimeout(() => {
    if (toast.parentNode) toast.remove();
    _toastActive = false;
    _processToastQueue();
  }, 3500);
}

// ============================================================================
// CONFIRM MODAL — replaces native confirm() with styled modal
// ============================================================================

function showConfirm({ title, message, confirmText, confirmClass, icon }) {
  return new Promise((resolve) => {
    const overlay = document.getElementById('confirmModalOverlay');
    const modal = document.getElementById('confirmModal');
    const iconEl = document.getElementById('confirmModalIcon');
    const titleEl = document.getElementById('confirmModalTitle');
    const msgEl = document.getElementById('confirmModalMessage');
    const okBtn = document.getElementById('confirmModalOk');
    const cancelBtn = document.getElementById('confirmModalCancel');

    if (!overlay || !modal) { resolve(false); return; }

    iconEl.innerHTML = icon || '<span class="fas fa-exclamation-triangle" style="color:var(--accent-warning)"></span>';
    titleEl.textContent = title || 'Are you sure?';
    msgEl.textContent = message || '';
    okBtn.textContent = confirmText || 'Confirm';
    okBtn.className = 'btn ' + (confirmClass || 'btn-danger');

    overlay.classList.add('open');
    modal.classList.add('open');
    document.body.style.overflow = 'hidden';

    function cleanup(result) {
      overlay.classList.remove('open');
      modal.classList.remove('open');
      document.body.style.overflow = '';
      okBtn.removeEventListener('click', onOk);
      cancelBtn.removeEventListener('click', onCancel);
      overlay.removeEventListener('click', onOverlay);
      resolve(result);
    }
    function onOk() { cleanup(true); }
    function onCancel() { cleanup(false); }
    function onOverlay(e) { if (e.target === overlay) cleanup(false); }

    okBtn.addEventListener('click', onOk);
    cancelBtn.addEventListener('click', onCancel);
    overlay.addEventListener('click', onOverlay);
  });
}

// ============================================================================
// RESIZE
// ============================================================================

function handleResize() {
  SML.isDesktop = window.innerWidth >= 768;
  if (SML.isDesktop) {
    // Close bottom sheet when resizing to desktop
    const sheet = document.getElementById('paramBottomSheet');
    const modOv = document.getElementById('paramModalOverlay');
    if (sheet) sheet.classList.remove('open');
    if (modOv) modOv.classList.remove('open');
  }
}

// ============================================================================
// WIFI / LED SAVE
// ============================================================================

async function saveWiFiConfig() {
  const ssid = document.getElementById('wifiSsid')?.value.trim();
  const pass = document.getElementById('wifiPass')?.value;
  if (!ssid) { showToast('Please enter an SSID', 'warning'); return; }
  if (!confirm(`Change WiFi to "${ssid}"? The device will reconnect and you may lose connection.`)) return;

  try {
    const params = new URLSearchParams();
    params.append('ssid', ssid);
    params.append('password', pass || '');

    const resp = await fetch('/save-wifi', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: params.toString()
    });
    const data = await resp.json();
    if (data.status === 'success') {
      showToast('WiFi saved. Reconnecting...', 'success');
    } else {
      showToast('Error: ' + (data.message || 'unknown'), 'error');
    }
  } catch (e) {
    showToast('Connection lost — device is restarting', 'error');
  }
}

// ============================================================================
// SKELETON HELPER
// ============================================================================

/** Sets textContent on an element and removes its skeleton loading state. */
function setDataValue(el, value, suffix) {
  if (!el) return;
  el.textContent = (value !== undefined && value !== null) ? (value + (suffix || '')) : '--';
  el.classList.remove('skeleton');
}

/** Sets innerHTML on an element and removes its skeleton loading state. */
function setDataHTML(el, html) {
  if (!el) return;
  el.innerHTML = html || '--';
  el.classList.remove('skeleton');
}
