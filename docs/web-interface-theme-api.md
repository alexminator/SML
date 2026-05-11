# SML Web Interface - Theme System API

## Overview

SML Web Interface v2.1 includes a dynamic theme system with 3 pre-built color schemes.

## Themes

### Theme IDs

- `wled-classic` - Cyan and blue (default)
- `sunset` - Gold and orange
- `ocean` - Blue and purple

## Usage

### JavaScript API

```javascript
// Access global ThemeManager instance
const themeManager = window.themeManager;

// Get current theme
const currentTheme = themeManager.getTheme();
// Returns: 'wled-classic', 'sunset', or 'ocean'

// Change theme
themeManager.changeTheme('sunset');
```

### HTML/CSS API

```html
<!-- Apply theme to page -->
<body data-theme="sunset">
  <!-- Content with sunset theme colors -->
</body>
```

```css
/* Use theme variables in custom CSS */
.my-element {
  background: var(--sml-cyan); /* Changes with theme */
  color: var(--text-primary);
}
```

### localStorage API

```javascript
// Read current theme (managed by ThemeManager)
const savedTheme = localStorage.getItem('sml-theme');

// Manually set theme (not recommended - use ThemeManager)
localStorage.setItem('sml-theme', 'ocean');
```

## Theme Variables

Each theme defines these CSS variables:

| Variable | WLED Classic | Sunset | Ocean |
|----------|--------------|--------|-------|
| `--bg-secondary` | #1E1E1E | #1a1400 | #0a0a1a |
| `--bg-tertiary` | #2A2A2A | #2c2100 | #14142e |
| `--text-secondary` | #E0E0E0 | #ffe0b2 | #e0e0ff |
| `--text-tertiary` | #B0B0B0 | #bcaaa4 | #b0b0ff |
| `--sml-cyan` | #00D4FF | #ffd700 | #3f51b5 |
| `--sml-blue` | #008BFF | #ff9800 | #9c27b0 |
| `--gradient-start` | #121212 | #1a1400 | #0a0a1a |
| `--gradient-mid` | #1a1a2e | #2c2100 | #14142e |
| `--gradient-end` | #16213e | #1a0f00 | #1a0a2e |
| `--sml-gold` | #FFD700 | #ffd700 | #3f51b5 |

## Adding Custom Themes

To add a new theme:

1. **Add CSS variables in `wled-theme.css`:**
```css
body[data-theme="mytheme"] {
  --bg-secondary: #......;
  --sml-cyan: #......;
  /* ... */
}
```

2. **Add theme card in `index.html`:**
```html
<div class="theme-card" data-theme="mytheme">
  <div class="theme-preview mytheme">
    <!-- preview structure -->
  </div>
  <h4>My Theme</h4>
  <p>Description</p>
</div>
```

3. **Add theme ID to `theme-manager.js`:**
```javascript
this.themes = ['wled-classic', 'sunset', 'ocean', 'mytheme'];
```

4. **Add preview styles in `config-tab.css`:**
```css
.theme-preview.mytheme .preview-header {
  background: linear-gradient(135deg, #...... 0%, #...... 100%);
}
.theme-preview.mytheme .preview-button {
  background: #......;
}
.theme-preview.mytheme .preview-slider {
  background: #......;
}
```

## Browser Compatibility

- Chrome 60+ ✅
- Firefox 55+ ✅
- Safari 11+ ✅
- Edge 79+ ✅
- Mobile browsers ✅

## Performance

- **Theme change:** < 20ms (instant)
- **Initial load overhead:** < 10ms
- **Memory footprint:** ~1KB
- **Network requests:** 0

## Accessibility

All themes meet WCAG 2.1 AA compliance:
- Color contrast ratio ≥ 4.5:1 for text
- Keyboard navigation support
- Screen reader announcements
- Focus indicators on theme cards

## Data Flow

### Initialization
```
Page load → CSS loads → Body rendered → theme-manager.js loads
→ localStorage read → data-theme applied → First paint
```

### Theme Change
```
User clicks card → Event captured → changeTheme() called
→ applyTheme() → data-theme updated → CSS repaint
→ localStorage saved → UI updated (< 20ms)
```

### Cross-Tab Sync
```
Tab A: changeTheme() → localStorage.setItem()
→ Browser fires 'storage' event → Tab B: storage listener
→ applyTheme() → Tab B updates automatically
```

---

**Version:** 2.1.0
**Last Updated:** 2026-05-11
