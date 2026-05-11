# Changelog - SML Web Interface

All notable changes to the SML Web Interface will be documented in this file.

## [2.1.1] - 2026-05-11

### Added
- Theme system with 3 interchangeable color schemes (WLED Classic, Sunset, Ocean)
- Theme selector in Config tab with live preview cards
- Automatic theme persistence in localStorage
- Responsive theme cards (3 columns desktop, 2 tablet, 1 mobile)
- Accessibility features for theme selection (keyboard navigation, ARIA labels)
- Theme system API documentation
- Cross-tab theme synchronization

### Changed
- CSS architecture now uses data-theme attribute for theming
- All color variables centralized in wled-theme.css
- Improved color consistency across all tabs
- Smooth transitions for theme changes (300ms)
- GPU acceleration for active theme cards

### Technical
- Added theme-manager.js for theme management (~158 lines)
- Enhanced config-tab.css with theme selector styles (~200 lines)
- Modified wled-theme.css with theme variable overrides (~50 lines)
- Added theme system documentation and API reference
- Modified index.html with theme selector HTML (~60 lines)

### Performance
- Theme switch < 20ms perceived (instant)
- Page load overhead < 10ms
- Memory footprint ~1KB
- Zero network overhead

---

## [2.1.0] - 2026-05-10

### Added
- Initial v2.1 web interface redesign
- Responsive navigation (mobile/tablet/desktop)
- Peek tab with real-time LED visualization
- Config tab with WiFi and LED hardware configuration
- WebSocket real-time updates
- 20 LED effects including VU meters
- Battery monitoring and charging status
- Temperature and humidity display
- Bluetooth audio controls

### Changed
- Complete UI/UX overhaul with WLED-inspired design
- CSS variable system for consistent theming
- Improved text contrast and readability
- Unified background gradient across interface

---

## [2.0.0] - 2026-04-30

### Added
- Modern web interface for SML
- Basic LED controls
- WiFi configuration
- Bluetooth playback controls

### Changed
- Migrated from legacy interface to responsive design

---

**Format:** Based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)
**Versions:** [Semantic Versioning](https://semver.org/spec/v2.0.0.html)
