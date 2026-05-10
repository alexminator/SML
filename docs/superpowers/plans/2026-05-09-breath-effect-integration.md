# Breath Effect Integration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Integrate the Breath effect (WLED-compatible) as effect ID 11 into the SML project, renumbering VU effects and special effects accordingly.

**Architecture:** Insert Breath effect between Comet (10) and VU effects, following the existing integration pattern. Changes span 4 files: main.cpp (logic), data.h (templates), main.js (mapping), and index.html (UI button).

**Tech Stack:** ESP32 Arduino Core, FastLED library, WebSocket communication, HTML/CSS/JavaScript web interface

---

## File Structure

**Files to be modified:**
- `src/main.cpp` - Add Breath include, parameter, function, and switch case; renumber existing effects 11-18 → 12-19
- `src/data.h` - Add BREATH_STATE template variable and breathState boolean
- `data/js/main.js` - Add Breathbutton mapping to effectMap; update VU and special effect mappings
- `data/index.html` - Add Breath button after Comet button in effects section

**No files created:** All changes are modifications to existing files following established patterns.

---

## Task 1: Add Breath Include and Parameter to main.cpp

**Files:**
- Modify: `src/main.cpp:323` (add include after existing effect includes)
- Modify: `src/main.cpp:170` (add parameter after existing effect parameters)

- [ ] **Step 1: Add Breath.h include after line 323**

```cpp
// Add this line after line 323 (after #include "Temp.h")
#include "Breath.h"
```

- [ ] **Step 2: Add Breath parameter after line 170 (after sinelonBeat)**

```cpp
// Breath Effect Parameters (WLED-compatible)
uint8_t breathSpeed = 128;  // Default: 128, range: 0-255 (breathing speed)
```

- [ ] **Step 3: Verify compilation**

Run: `arduino-cli compile --fqbn esp32:esp32:esp32`
Expected: Compilation succeeds with no errors

- [ ] **Step 4: Commit changes**

```bash
git add src/main.cpp
git commit -m "feat: add Breath include and parameter to main.cpp"
```

---

## Task 2: Add runBreath() Function to main.cpp

**Files:**
- Modify: `src/main.cpp:490` (add runBreath function after runComet function)

- [ ] **Step 1: Add runBreath() function after line 490**

```cpp
void runBreath()
{
    Breath breath = Breath();
    breath.runPattern();
}
```

- [ ] **Step 2: Verify function follows existing pattern**

The new function should follow the exact same pattern as runComet(), runFire(), etc. - create object instance, call runPattern(), no return value.

- [ ] **Step 3: Verify compilation**

Run: `arduino-cli compile --fqbn esp32:esp32:esp32`
Expected: Compilation succeeds, linking resolves Breath::runPattern()

- [ ] **Step 4: Commit changes**

```bash
git add src/main.cpp
git commit -m "feat: add runBreath() function to main.cpp"
```

---

## Task 3: Update Switch Statement with Breath Case

**Files:**
- Modify: `src/main.cpp:576-602` (replace entire switch/case section)

- [ ] **Step 1: Replace case 10 and add case 11 for Breath**

Find the existing switch case around line 576. Replace the entire section from case 10 through case 18 with:

```cpp
        case 10:
            runComet();
            break;
        case 11:  // NEW - Breath effect
            runBreath();
            break;
        case 12:  // Was case 11 - RainbowVU
            runRainbowVU();
            break;
        case 13:  // Was case 12 - OldVU
            runOldVU();
            break;
        case 14:  // Was case 13 - RainbowHueVU
            runRainbowHueVU();
            break;
        case 15:  // Was case 14 - RippleVU
            runRippleVU();
            break;
        case 16:  // Was case 15 - ThreebarsVU
            runThreebarsVU();
            break;
        case 17:  // Was case 16 - OceanVU
            runOceanVU();
            break;
        case 18:  // Was case 17 - Temperature
            runTemperature();
            break;
        case 19:  // Was case 18 - Battery
            runBattery();
            break;
```

- [ ] **Step 2: Verify switch statement logic**

Confirm that:
- Cases 0-10 remain unchanged
- Case 11 is the new Breath effect
- Cases 12-19 are the renumbered VU and special effects
- Default case remains unchanged

- [ ] **Step 3: Verify compilation**

Run: `arduino-cli compile --fqbn esp32:esp32:esp32`
Expected: Clean compilation with no warnings about duplicate case values

- [ ] **Step 4: Commit changes**

```bash
git add src/main.cpp
git commit -m "feat: integrate Breath as case 11, renumber effects 12-19"
```

---

## Task 4: Add Breath Template Integration to main.cpp

**Files:**
- Modify: `src/main.cpp` (add BREATH_STATE to enum Status and processor function)

**CRITICAL:** This codebase uses enum Status + processor() function, NOT #define macros for template variables.

- [ ] **Step 1: Add BREATH_STATE to enum Status (around line 909)**

Find the enum Status definition in main.cpp. Add BREATH_STATE after the last effect state:

```cpp
enum Status {
    COLOR,
    // ... existing states
    COMET_STATE,
    BREATH_STATE,  // NEW - Breath effect template variable
    // ... VU and special states
};
```

- [ ] **Step 2: Add BREATH_STATE case to processor() function (around line 924)**

Find the processor() function. Add a case for BREATH_STATE:

```cpp
const char* processor(const String &var) {
    static char buffer[64];
    switch (status) {
        // ... existing cases
        case BREATH_STATE:
            return (stripLed.effectId == 11 && stripLed.powerState) ? "on" : "off";
        // ... other cases
    }
    return String();
}
```

- [ ] **Step 3: Add breathStatus to effectNames array (around line 1125)**

Find the effectNames array and add Breath status:

```cpp
const char* effectNames[] = {
    "Fire", "MovingDot", "RainbowBeat", // ... existing effects
    "Comet", "Breath",  // Add "Breath" after "Comet"
    "RainbowVU", "OldSkoolVU", // ... VU effects
    // ... special effects
};
```

- [ ] **Step 4: Verify template system integration**

Confirm that:
- BREATH_STATE enum value follows the existing pattern
- processor() function returns correct "on"/"off" for effectId 11
- effectNames array includes "Breath" in the correct position

- [ ] **Step 5: Verify compilation**

Run: `arduino-cli compile --fqbn esp32:esp32:esp32`
Expected: Compilation succeeds, template system properly configured

- [ ] **Step 6: Commit changes**

```bash
git add src/main.cpp
git commit -m "feat: add Breath template integration (enum Status + processor)"
```

---

## Task 5: Update effectMap in main.js

**Files:**
- Modify: `data/js/main.js:299-318` (update effectMap object)

- [ ] **Step 1: Add Breathbutton mapping to effectMap**

Find the effectMap object around line 299. Add the Breathbutton entry after Cometbutton:

```javascript
const effectMap = {
    Firebutton: 1,
    MovingDotbutton: 2,
    RainbowBeatbutton: 3,
    RWBbutton: 4,
    Ripplebutton: 5,
    Twinklebutton: 6,
    Ballsbutton: 7,
    Jugglebutton: 8,
    Sinelonbutton: 9,
    Cometbutton: 10,
    Breathbutton: 11,  // NEW
    RainbowVU: 12,     // Was 11
    OldSkoolVU: 13,    // Was 12
    RainbowHueVU: 14,  // Was 13
    RippleVU: 15,      // Was 14
    ThreebarsVU: 16,   // Was 15
    OceanVU: 17,       // Was 16
    TempNEO: 18,       // Was 17
    BattNEO: 19        // Was 18
};
```

- [ ] **Step 2: Verify effectMap consistency**

Confirm that:
- Breathbutton maps to 11
- VU effects now map to 12-17
- Special effects now map to 18-19
- No duplicate or conflicting mappings

- [ ] **Step 3: Test JavaScript syntax**

Run: `node -c data/js/main.js`
Expected: No syntax errors

- [ ] **Step 4: Commit changes**

```bash
git add data/js/main.js
git commit -m "feat: add Breath to effectMap and update VU/special mappings"
```

---

## Task 6: Add Breath Button to index.html

**Files:**
- Modify: `data/index.html:150-160` (add Breath button after Comet button)

- [ ] **Step 1: Add Breath button HTML after Comet button**

Find the Comet button section around line 150. Add the Breath button immediately after:

```html
            <h3 class="comet">Comet</h3>
            <div id="Cometbutton" class="%COMET_STATE%">
              <span class="button">
                <span class="slide"></span>
              </span>
            </div>
            <h3 class="breath">Breath</h3>
            <div id="Breathbutton" class="%BREATH_STATE%">
              <span class="button">
                <span class="slide"></span>
              </span>
            </div>
```

- [ ] **Step 2: Verify HTML structure**

Confirm that:
- Breath button follows exact same structure as other effect buttons
- Uses %BREATH_STATE% template variable
- Has proper class names (breath, Breathbutton)
- Nested correctly in effects-button div

- [ ] **Step 3: Test HTML validity**

Run: Open data/index.html in browser, check for no console errors
Expected: Clean HTML with no malformed elements

- [ ] **Step 4: Commit changes**

```bash
git add data/index.html
git commit -m "feat: add Breath button to web interface"
```

---

## Task 7: Full Integration Test

**Files:**
- Test: All modified files working together
- Verify: Complete workflow from UI to ESP32

- [ ] **Step 1: Full project compilation**

Run: `pio run` (or `arduino-cli compile --fqbn esp32:esp32:esp32`)
Expected: Clean build with no errors or warnings

- [ ] **Step 2: Verify Breath.h integration**

Check that:
- Breath.h is properly included
- breathSpeed parameter is defined (default 128)
- runBreath() function is accessible
- Case 11 calls runBreath()

- [ ] **Step 3: Test web interface locally**

Run: Open data/index.html in browser
Check:
- Breath button appears after Comet button
- Button has correct styling
- Template %BREATH_STATE% shows "off" initially

- [ ] **Step 4: Verify JavaScript mapping**

Open browser console and test:
```javascript
console.log(effectMap['Breathbutton']);  // Should output: 11
```

Expected: effectMap correctly maps Breathbutton to 11

- [ ] **Step 5: Final commit if all tests pass**

```bash
git add docs/superpowers/plans/2026-05-09-breath-effect-integration.md
git commit -m "docs: add Breath effect integration implementation plan"
```

---

## Task 8: Documentation Updates

**Files:**
- Modify: `README.md` (add Breath to effects list)
- Modify: `README_es.md` (add Breath to Spanish effects list)
- Modify: `CLAUDE.md` (update effect IDs documentation)

- [ ] **Step 1: Update README.md effects list**

Find the effects section in README.md and add Breath:

```markdown
- **11**: Breath (WLED-compatible breathing effect)
- **12-17**: VU Meter effects (renumbered)
- **18**: Temperature-based color
- **19**: Battery visualization
```

- [ ] **Step 2: Update README_es.md effects list**

Add Spanish translation:

```markdown
- **11**: Breath (efecto de respiración compatible WLED)
- **12-17**: Efectos VU Meter (renumerados)
- **18**: Color basado en temperatura
- **19**: Visualización de batería
```

- [ ] **Step 3: Update CLAUDE.md effect system**

Update the effect system section in CLAUDE.md:

```markdown
## Effect System

The project includes multiple LED effects organized by ID:

- **0-10**: Base effects (MovingDot, Fire, RainbowBeat, etc.)
- **11**: Breath (WLED-compatible breathing effect)
- **12-17**: VU Meter effects (music visualization)
- **18**: Temperature-based color
- **19**: Battery visualization
```

- [ ] **Step 4: Verify documentation accuracy**

Confirm that:
- All effect IDs are correct
- Descriptions are accurate
- Both English and Spanish versions updated
- No conflicting information

- [ ] **Step 5: Commit documentation updates**

```bash
git add README.md README_es.md CLAUDE.md
git commit -m "docs: update effect documentation for Breath integration"
```

---

## Task 9: Final Verification and Testing

**Files:**
- Test: Complete system integration
- Verify: All functionality works as expected

- [ ] **Step 1: Upload to ESP32 and test**

Run: `pio run --target upload` (or use Arduino CLI)
Expected: Firmware uploads successfully

- [ ] **Step 2: Test Breath effect activation**

Using web interface:
1. Open ESP32 web interface
2. Click "Breath" button
3. Verify LEDs show breathing pattern
4. Verify button shows "on" state

Expected: Breath effect activates, smooth breathing animation visible

- [ ] **Step 3: Test effect switching**

Test transitions:
1. Breath → Fire (effect 11 → 1)
2. Fire → Breath (effect 1 → 11)
3. Breath → RainbowVU (effect 11 → 12)

Expected: Clean transitions, no crashes or glitches

- [ ] **Step 4: Test VU effects with new IDs**

Activate each VU effect:
1. RainbowVU (was 11, now 12)
2. OldVU (was 12, now 13)
3. RainbowHueVU (was 13, now 14)
4. RippleVU (was 14, now 15)
5. ThreebarsVU (was 15, now 16)
6. OceanVU (was 16, now 17)

Expected: All VU effects work correctly with new IDs

- [ ] **Step 5: Test special effects with new IDs**

Activate special effects:
1. Temperature (was 17, now 18)
2. Battery (was 18, now 19)

Expected: Both special effects work correctly with new IDs

- [ ] **Step 6: Verify base effects unchanged**

Test effects 0-10:
1. Simple Color (0)
2. Fire (1)
3. MovingDot (2)
4. RainbowBeat (3)
5. RedWhiteBlue (4)
6. Ripple (5)
7. Twinkle (6)
8. Bouncing Balls (7)
9. Juggle (8)
10. Sinelon (9)
11. Comet (10)

Expected: All base effects work exactly as before

- [ ] **Step 7: Final commit and tag**

```bash
git add .
git commit -m "feat: complete Breath effect integration (WLED-compatible)"
git tag -a v0.11.0 -m "Add Breath effect as ID 11, renumber VU/special effects"
```

---

## Success Criteria

**Integration Complete When:**
- ✅ Breath effect executes with WLED algorithm (sin8-based breathing)
- ✅ Web button activates/deactivates effect correctly
- ✅ Renumbered VU effects (12-17) function properly
- ✅ Special effects (18-19) work with new IDs
- ✅ Base effects (0-10) remain unaffected
- ✅ Code compiles without warnings
- ✅ No memory leaks or crashes
- ✅ Documentation updated accurately

**Testing Checklist:**
- ✅ Compilation successful
- ✅ Breath button appears in web interface
- ✅ WebSocket message {effectId: 11} triggers Breath effect
- ✅ Visual breathing pattern displays correctly
- ✅ All effect transitions work smoothly
- ✅ No regression in existing functionality

---

## Notes

- Breath effect uses WLED mode_breath algorithm with sin8() for smooth intensity
- Default breathSpeed is 128 (mid-range), range 0-255
- Color cycling enabled: myhue++ every 1 second
- Effect positioned with "soft" effects (Twinkle, Ripple, Comet)
- Template system ensures proper state management
- JavaScript effectMap enables button-to-effect ID mapping

## References

- Spec: `docs/superpowers/specs/2026-05-09-breath-effect-integration-design.md`
- WLED mode_breath implementation reference
- Existing effect integration patterns (Fire, Comet, etc.)
- SML project CLAUDE.md guidelines
