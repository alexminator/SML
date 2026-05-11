/**
 * ThemeManager - Sistema de gestión de temas SML Web Interface
 *
 * Gestiona 3 temas de color (WLED Classic, Sunset, Ocean)
 * con persistencia en localStorage y cambio dinámico vía data-theme attribute.
 *
 * @class ThemeManager
 * @example
 * const theme = new ThemeManager();
 * theme.changeTheme('sunset');
 * console.log(theme.getTheme()); // 'sunset'
 */

// Initialize logger
const themeLogger = createLogger('Theme');

class ThemeManager {
  constructor() {
    this.currentTheme = this.loadTheme();
    this.themes = ['wled-classic', 'sunset', 'ocean'];
    this.body = document.body;
    this.themeCards = null;

    // Only initialize if theme selector exists in DOM
    if (document.querySelector('.theme-selector')) {
      this.init();
    }
  }

  init() {
    // Apply saved theme synchronously (before first paint)
    this.applyTheme(this.currentTheme, false);

    // Setup event listeners after DOM ready
    if (document.readyState === 'loading') {
      document.addEventListener('DOMContentLoaded', () => {
        this.setupThemeCards();
        this.setupStorageSyncListener();
      });
    } else {
      this.setupThemeCards();
      this.setupStorageSyncListener();
    }
  }

  loadTheme() {
    try {
      const saved = localStorage.getItem('sml-theme');
      // Validate theme is in allowed list
      if (saved && this.themes.includes(saved)) {
        return saved;
      }
    } catch (e) {
      // localStorage not available (private mode)
      themeLogger.warn('localStorage not available, using default theme');
    }
    return 'wled-classic';
  }

  saveTheme(theme) {
    try {
      localStorage.setItem('sml-theme', theme);
      this.currentTheme = theme;
    } catch (e) {
      // Silently fail in private mode
      if (window.DEBUG_MODE) {
        themeLogger.warn('localStorage write failed:', e);
      }
    }
  }

  applyTheme(theme, withTransition = false) {
    // Validate theme
    if (!this.themes.includes(theme)) {
      themeLogger.warn(`Invalid theme "${theme}", using default`);
      theme = 'wled-classic';
    }

    // Apply transition if requested
    if (withTransition) {
      this.body.style.transition = 'background-color 0.3s ease, color 0.3s ease';
    }

    // Apply theme to body
    this.body.setAttribute('data-theme', theme);
    this.currentTheme = theme;

    // Update theme cards UI if already cached
    if (this.themeCards) {
      this.updateThemeCards(theme);
    }

    // Remove transition after applying
    if (withTransition) {
      setTimeout(() => {
        this.body.style.transition = '';
      }, 300);
    }
  }

  setupThemeCards() {
    // Cache theme cards DOM query
    this.themeCards = document.querySelectorAll('.theme-card');

    // Use event delegation on container
    const selector = document.querySelector('.theme-selector');
    if (selector) {
      selector.addEventListener('click', (e) => {
        const card = e.target.closest('.theme-card');
        if (card) {
          const theme = card.getAttribute('data-theme');
          this.changeTheme(theme);
        }
      });
    }

    // Update initial state
    this.updateThemeCards(this.currentTheme);
  }

  setupStorageSyncListener() {
    // Sync theme across tabs/windows
    window.addEventListener('storage', (e) => {
      if (e.key === 'sml-theme' && e.newValue) {
        this.applyTheme(e.newValue);
      }
    });
  }

  changeTheme(theme) {
    // Don't change if already active
    if (theme === this.currentTheme) return;

    // Apply theme with transition
    this.applyTheme(theme, true);

    // Save to localStorage
    this.saveTheme(theme);
  }

  updateThemeCards(activeTheme) {
    if (!this.themeCards) return;

    this.themeCards.forEach(card => {
      const cardTheme = card.getAttribute('data-theme');
      const isActive = cardTheme === activeTheme;

      // Update visual state
      card.classList.toggle('active', isActive);

      // Update accessibility state
      card.setAttribute('aria-checked', isActive.toString());
    });
  }

  getTheme() {
    return this.currentTheme;
  }
}

// Create global instance when script loads
window.themeManager = new ThemeManager();
