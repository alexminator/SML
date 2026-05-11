/**
 * TabsManager - Tab navigation and sidebar management
 *
 * Handles tab switching, sidebar state persistence, and responsive navigation
 * for the SML web interface.
 */

class TabsManager {
  constructor() {
    this.tabs = [
      { id: 'lamp', name: 'Lamp', icon: 'lightbulb' },
      { id: 'music', name: 'Music', icon: 'music' },
      { id: 'peek', name: 'Peek', icon: 'eye' },
      { id: 'weather', name: 'Weather', icon: 'cloud-sun' },
      { id: 'battery', name: 'Battery', icon: 'battery-three-quarters' },
      { id: 'config', name: 'Config', icon: 'cog' }
    ];

    this.currentTab = 'lamp';
    this.sidebarCollapsed = false;

    this.SIDEBAR_STATE_KEY = 'sml_sidebar_collapsed';
  }

  /**
   * Initialize the tabs manager
   */
  init() {
    this.setupNavigation();
    this.setupEventListeners();
    this.setupSidebarToggle();
    this.restoreSidebarState();

    // Show initial tab
    this.switchTab(this.currentTab);
  }

  /**
   * Setup tab navigation elements
   */
  setupNavigation() {
    // Navigation already exists in HTML, just setup event handlers
    // Mobile bottom nav and desktop sidebar are already structured
  }

  /**
   * Setup event listeners for navigation
   */
  setupEventListeners() {
    // Mobile bottom nav clicks
    const mobileNavItems = document.querySelectorAll('.mobile-bottom-nav .nav-item');
    mobileNavItems.forEach(item => {
      item.addEventListener('click', (e) => {
        e.preventDefault();
        const tabId = item.dataset.tab;
        this.switchTab(tabId);
      });
    });

    // Desktop sidebar clicks
    const sidebarItems = document.querySelectorAll('.desktop-sidebar .sidebar-item');
    sidebarItems.forEach(item => {
      item.addEventListener('click', (e) => {
        e.preventDefault();
        const tabId = item.dataset.tab;
        this.switchTab(tabId);
      });
    });

    // Listen for custom tab switch events
    document.addEventListener('switchTab', (e) => {
      if (e.detail && e.detail.tabId) {
        this.switchTab(e.detail.tabId);
      }
    });

    // Handle keyboard navigation
    document.addEventListener('keydown', (e) => {
      // Ctrl/Cmd + number keys to switch tabs
      if ((e.ctrlKey || e.metaKey) && e.key >= '1' && e.key <= '6') {
        e.preventDefault();
        const tabIndex = parseInt(e.key) - 1;
        if (tabIndex < this.tabs.length) {
          this.switchTab(this.tabs[tabIndex].id);
        }
      }
    });
  }

  /**
   * Setup sidebar toggle functionality
   */
  setupSidebarToggle() {
    const toggleButton = document.querySelector('.sidebar-toggle');

    if (toggleButton) {
      toggleButton.addEventListener('click', () => {
        this.toggleSidebar();
      });
    }
  }

  /**
   * Switch to a specific tab
   * @param {string} tabId - The ID of the tab to switch to
   */
  switchTab(tabId) {
    // Validate tab ID
    if (!this.tabs.find(tab => tab.id === tabId)) {
      console.warn(`Invalid tab ID: ${tabId}`);
      return;
    }

    // Store current tab
    const previousTab = this.currentTab;
    this.currentTab = tabId;

    // Hide all tabs and show the selected one
    this.hideAllTabs();
    this.showTab(tabId);

    // Update active state
    this.updateActiveState(tabId);

    // Dispatch tab change event
    document.dispatchEvent(new CustomEvent('tabChanged', {
      detail: {
        tabId: tabId,
        previousTab: previousTab
      }
    }));
  }

  /**
   * Hide all tab content panels
   */
  hideAllTabs() {
    const tabPanels = document.querySelectorAll('.tab-content');
    tabPanels.forEach(panel => {
      panel.classList.remove('active');
      panel.style.display = 'none';
    });
  }

  /**
   * Show a specific tab content panel
   * @param {string} tabId - The ID of the tab to show
   */
  showTab(tabId) {
    // Map tab IDs to content IDs
    const tabIdMap = {
      'lamp': 'tab-lamp',
      'music': 'tab-music',
      'peek': 'tab-peek',
      'weather': 'tab-temp',
      'battery': 'tab-battery',
      'config': 'tab-config'
    };

    const contentId = tabIdMap[tabId] || `tab-${tabId}`;
    const panel = document.getElementById(contentId);
    if (panel) {
      panel.classList.add('active');
      panel.style.display = 'block';
    }
  }

  /**
   * Update the active state of navigation items
   * @param {string} tabId - The ID of the active tab
   */
  updateActiveState(tabId) {
    // Update mobile bottom nav
    const mobileNavItems = document.querySelectorAll('.mobile-bottom-nav .nav-item');
    mobileNavItems.forEach(item => {
      if (item.dataset.tab === tabId) {
        item.classList.add('active');
        item.setAttribute('aria-current', 'page');
      } else {
        item.classList.remove('active');
        item.removeAttribute('aria-current');
      }
    });

    // Update desktop sidebar
    const sidebarItems = document.querySelectorAll('.desktop-sidebar .sidebar-item');
    sidebarItems.forEach(item => {
      if (item.dataset.tab === tabId) {
        item.classList.add('active');
        item.setAttribute('aria-current', 'page');
      } else {
        item.classList.remove('active');
        item.removeAttribute('aria-current');
      }
    });
  }

  /**
   * Toggle sidebar collapsed state
   */
  toggleSidebar() {
    this.sidebarCollapsed = !this.sidebarCollapsed;

    const sidebar = document.querySelector('.desktop-sidebar');
    const mainContent = document.querySelector('.main');

    if (this.sidebarCollapsed) {
      sidebar.classList.add('collapsed');
      mainContent.classList.add('expanded');
    } else {
      sidebar.classList.remove('collapsed');
      mainContent.classList.remove('expanded');
    }

    // Save state to localStorage
    localStorage.setItem(this.SIDEBAR_STATE_KEY, this.sidebarCollapsed);

    // Dispatch sidebar toggle event
    document.dispatchEvent(new CustomEvent('sidebarToggled', {
      detail: {
        collapsed: this.sidebarCollapsed
      }
    }));
  }

  /**
   * Restore sidebar state from localStorage
   */
  restoreSidebarState() {
    const savedState = localStorage.getItem(this.SIDEBAR_STATE_KEY);

    if (savedState !== null) {
      this.sidebarCollapsed = savedState === 'true';

      const sidebar = document.querySelector('.desktop-sidebar');
      const mainContent = document.querySelector('.main');

      if (this.sidebarCollapsed) {
        sidebar.classList.add('collapsed');
        mainContent.classList.add('expanded');
      }
    }
  }

  /**
   * Get the current active tab
   * @returns {string} The current tab ID
   */
  getCurrentTab() {
    return this.currentTab;
  }

  /**
   * Get sidebar collapsed state
   * @returns {boolean} Whether the sidebar is collapsed
   */
  isSidebarCollapsed() {
    return this.sidebarCollapsed;
  }
}

// Create global instance
const tabsManager = new TabsManager();

// Initialize on DOM content loaded
document.addEventListener('DOMContentLoaded', () => {
  tabsManager.init();
});

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
  module.exports = { TabsManager, tabsManager };
}
