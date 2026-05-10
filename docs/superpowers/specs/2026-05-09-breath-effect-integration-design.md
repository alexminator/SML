# Breath Effect Integration Design

**Date:** 2026-05-09
**Author:** Claude + User
**Status:** Approved
**Related:** WLED Effects Migration Project

## Overview

Integrate the Breath effect (WLED-compatible algorithm) into the SML project as effect ID 11, following the existing integration pattern and renumbering VU effects and special effects accordingly.

## Objectives

1. Add Breath effect as case 11 in the effect switch statement
2. Renumber VU effects (11-16 → 12-17) and special effects (17-18 → 18-19)
3. Add Breath button to web interface with proper template variable
4. Maintain backward compatibility with existing effects
5. Follow WLED algorithm and parameter structure

## Current State

- **Effects 0-10:** Base effects (unchanged)
- **Effects 11-16:** VU meter effects
- **Effects 17-18:** Special effects (Temperature, Battery)
- **Breath effect:** Already implemented in `src/Breath.h` with WLED algorithm

## Target State

- **Effects 0-10:** Base effects (unchanged)
- **Effect 11:** **Breath** (NEW)
- **Effects 12-17:** VU meter effects (renumbered)
- **Effects 18-19:** Special effects (renumbered)

## Architecture

### Component Structure

```
User Interface Layer
├── index.html - Breath button (%BREATH_STATE%)
└── main.js - effectMap entry (Breathbutton: 11)

Template Layer
└── data.h - BREATH_STATE template variable

Business Logic Layer
├── main.cpp - runBreath() function + case 11
└── Breath.h - WLED algorithm (already exists)
```

### Effect Mapping

| Old ID | Effect | New ID | Change |
|--------|--------|--------|--------|
| - | - | 11 | **NEW: Breath** |
| 11 | RainbowVU | 12 | +1 |
| 12 | OldVU | 13 | +1 |
| 13 | RainbowHueVU | 14 | +1 |
| 14 | RippleVU | 15 | +1 |
| 15 | ThreebarsVU | 16 | +1 |
| 16 | OceanVU | 17 | +1 |
| 17 | Temperature | 18 | +1 |
| 18 | Battery | 19 | +1 |

## Implementation Details

### 1. Logic Layer (main.cpp)

**Function Addition:**
```cpp
void runBreath()
{
    Breath breath = Breath();
    breath.runPattern();
}
```

**Switch/Case Modifications:**
- Add case 11 for Breath effect
- Renumber cases 12-19 (VU + special effects)
- Maintain existing effect logic (0-10 unchanged)

**Parameter Initialization:**
```cpp
// Breath Effect Parameters (WLED-compatible)
uint8_t breathSpeed = 128;  // Default: 128, range: 0-255
```

### 2. Template Layer (data.h)

**Template Variable:**
```cpp
#define BREATH_STATE breathState ? "on" : "off"
```

**State Variable:**
```cpp
bool breathState = false;
```

### 3. Web Interface Integration

**JavaScript (main.js):**
```javascript
const effectMap = {
    // ... existing mappings
    Cometbutton: 10,
    Breathbutton: 11,  // NEW
    // ... VU and special effects updated accordingly
};
```

**HTML Button (index.html):**
```html
<h3 class="breath">Breath</h3>
<div id="Breathbutton" class="%BREATH_STATE%">
  <span class="button">
    <span class="slide"></span>
  </span>
</div>
```

## Data Flow

### Activation Flow

1. **User Action:** Click "Breath" button in web interface
2. **WebSocket Message:** `{effectId: 11}`
3. **ESP32 Processing:**
   - Receive `effectId = 11`
   - Switch executes `runBreath()`
   - Breath effect starts with default parameters
4. **Visual Output:** LEDs show breathing pattern with current color

### State Management

- **Initial State:** `breathState = false`, `effectId = 0`
- **After Activation:** `breathState = true`, `effectId = 11`
- **Template Update:** `%BREATH_STATE%` → `"on"`
- **Button Visual:** Shows active state

## Error Handling

### Validation Checks

- ✅ Breath.h exists and compiles
- ✅ Parameter `breathSpeed` has default value (128)
- ✅ WLED algorithm tested in Breath.h
- ✅ Switch case integration follows existing pattern

### Fallback Behavior

- **Breath.h Missing:** Effect won't execute, no crash
- **effectMap Missing:** JavaScript defaults to `effectId: 0` (simple color)
- **Template Syntax Error:** Caught at compile time

## Testing Strategy

### Unit Tests

1. **Compilation Test:** Verify Breath.h compiles without errors
2. **Integration Test:** Confirm `runBreath()` integrates correctly
3. **Renumbering Test:** Validate VU effects (12-17) work correctly
4. **Special Effects Test:** Confirm Temperature (18) and Battery (19) function

### Integration Tests

1. **Full Compilation:** Build project with Arduino skill
2. **Web Interface:** Verify Breath button appears and functions
3. **WebSocket Test:** Send `{effectId: 11}` and confirm execution
4. **Visual Test:** Confirm breathing pattern displays correctly

### Regression Tests

1. **Base Effects (0-10):** Verify no functionality changes
2. **VU Effects (12-17):** Test all VU modes with new IDs
3. **Special Effects (18-19):** Confirm Temperature and Battery work
4. **Effect Switching:** Test transitions between different effects

## Performance Considerations

### Memory Usage

- **Breath Effect:** ~100 bytes (negligible impact)
- **Template Variable:** ~4 bytes
- **Total Impact:** Minimal, well within ESP32 capabilities

### CPU Usage

- **Breath Algorithm:** Optimized with sin8() and 50 FPS
- **Task Delay:** 20ms (vTaskDelay)
- **Impact:** Low, similar to existing effects

### Power Consumption

- **AC Mode:** No change (full power)
- **Battery Mode:** No additional overhead
- **Sleep Mode:** Effect follows power management rules

## Success Criteria

### Functional Requirements

- ✅ Breath effect executes correctly with WLED algorithm
- ✅ Web button activates/deactivates effect
- ✅ Renumbered VU effects (12-17) function properly
- ✅ Special effects (18-19) work with new IDs
- ✅ Existing effects (0-10) unaffected

### Technical Requirements

- ✅ Code compiles without warnings
- ✅ Memory safe (no leaks, buffer overflows)
- ✅ Follows existing integration pattern
- ✅ Documentation updated appropriately

### User Experience

- ✅ Breath button positioned logically (after Comet)
- ✅ Effect name descriptive ("Breath")
- ✅ Visual feedback clear (button state)
- ✅ No performance degradation

## Risk Assessment

### Risk 1: Renumbering Impact

**Impact:** Medium - May affect existing workflows
**Probability:** Low - Only affects 8 effects
**Mitigation:** Clear documentation, easy reversion
**Contingency:** Git revert if issues arise

### Risk 2: Template Syntax

**Impact:** Low - Easy to detect and fix
**Probability:** Very Low - Following existing pattern
**Mitigation:** Copy existing template syntax exactly
**Contingency:** Syntax fix in data.h

### Risk 3: JavaScript Integration

**Impact:** Medium - Button won't function
**Probability:** Low - Simple pattern copy
**Mitigation:** Test in browser console
**Contingency:** Debug JavaScript and fix mapping

## Dependencies

### Required Components

- ✅ `src/Breath.h` (already exists)
- ✅ FastLED library (already integrated)
- ✅ WebSocket system (functional)
- ✅ Template system (operational)

### System Requirements

- ✅ ESP32 with sufficient memory
- ✅ LED strip WS2812B (24 LEDs)
- ✅ No additional hardware required

### Software Requirements

- ✅ Arduino CLI / PlatformIO
- ✅ ESP32 Arduino Core v2.0+
- ✅ FastLED v3.5.0+

## Documentation Plan

### Technical Documentation

- Add comments explaining WLED algorithm reference
- Update CLAUDE.md with Breath effect (case 11)
- Document parameter structure (breathSpeed: 128)

### User Documentation

- Update README.md with Breath in effects list
- Update README_es.md with Spanish translation
- Web interface button is self-explanatory

### Change Communication

- Commit message: "feat: add Breath effect (WLED-compatible)"
- Git diff shows clear, isolated changes
- Easy code review by other developers

## Timeline Estimate

- **Implementation:** 30-45 minutes
- **Testing:** 15-20 minutes
- **Documentation:** 10 minutes
- **Total:** ~1 hour

## Notes

- Breath effect follows WLED mode_breath algorithm
- Uses sin8() for smooth breathing intensity (0 → 255 → 0)
- Color cycling enabled (myhue++ every 1 second)
- Default speed: 128 (mid-range breathing speed)
- Effect positioned with "soft" effects (Twinkle, Ripple, Comet)

## References

- WLED mode_breath implementation
- Existing effect integration pattern (Fire, Comet, etc.)
- WLED Effects Migration Project documentation
- SML project CLAUDE.md guidelines
