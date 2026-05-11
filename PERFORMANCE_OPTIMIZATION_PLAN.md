# Performance Optimization Plan

**SML Web Interface v2.1**
**Created:** 2026-05-10
**Status:** ⏳ Implementation Pending

---

## Current Performance Baseline

### Target Metrics (Pre-Optimization)
- **Canvas Rendering:** 60 FPS (desktop), 30 FPS (mobile)
- **WebSocket Updates:** 20 FPS (50ms interval)
- **First Contentful Paint:** ~1.5s
- **Time to Interactive:** ~3s
- **Memory Heap:** ~30-50MB
- **ESP32 Stack:** ~4KB free (acceptable)

---

## Optimization Opportunities

### 1. Canvas Rendering Optimizations

#### Issue 1.1: Gradient Recalculation on Every Frame
**Problem:** `createRadialGradient()` called 60 times per frame for each LED

**Current Code (peek-render.js):**
```javascript
for (let i = 0; i < this.currentLEDCount; i++) {
  const gradient = this.ctx.createRadialGradient(x, y, 0, x, y, ledSize);
  // ...
}
```

**Impact:** High CPU usage, especially on mobile

**Solution:** Cache gradient or use simpler rendering

**Optimization Options:**
1. **Pre-render LED sprites** to offscreen canvas
2. **Use solid colors** with single shadow for glow
3. **Batch gradient creation** for same-sized LEDs

**Expected Improvement:** 20-30% FPS increase on mobile

**Implementation Priority:** High

---

#### Issue 1.2: Excessive Object Allocation
**Problem:** New objects created each frame (`{r,g,b}`, gradients, paths)

**Current Code:**
```javascript
const color = this.leds[ledIndex];
// Creates new object every render call
```

**Solution:** Reuse objects where possible

**Optimization:**
```javascript
// Pre-allocate color object
const tempColor = { r: 0, g: 0, b: 0 };

// In render loop
tempColor.r = this.leds[ledIndex].r;
tempColor.g = this.leds[ledIndex].g;
tempColor.b = this.leds[ledIndex].b;
```

**Expected Improvement:** Reduced GC pressure, smoother FPS

**Implementation Priority:** Medium

---

#### Issue 1.3: Unnecessary Canvas Clears
**Problem:** Full canvas clear every frame, even for small changes

**Solution:** Dirty rectangle tracking (only clear changed LEDs)

**Complexity:** High (LED positions change in Circle mode)

**Expected Improvement:** Minimal for full-screen updates

**Implementation Priority:** Low (not worth complexity)

---

### 2. WebSocket Efficiency

#### Issue 2.1: JSON Payload Size
**Problem:** LED data as JSON array of objects: `[{"r":0,"g":0,"b":0},...]`

**Current Size (60 LEDs):**
- JSON overhead: ~2KB per message
- 20 FPS = 40KB/s bandwidth
- ESP32 memory: ~4KB per message buffer

**Solutions:**

**Option A: Compact JSON Format**
```json
{"leds":[[0,0,0],[255,0,0],...],"rc":24,"pc":60,"ef":"Fire"}
```
**Reduction:** ~30% smaller (1.4KB for 60 LEDs)

**Option B: Binary Format (MessagePack)**
- Requires MessagePack library
- ~50% smaller than JSON
- **Reduction:** ~1KB for 60 LEDs

**Option C: Custom Binary Protocol**
```javascript
// Binary format: [R,G,B, R,G,B, ...]
// Prepend: [previewCount(1), realCount(2), effectNameLen(1), effectName(N)]
```
**Reduction:** ~60% smaller (800 bytes for 60 LEDs)

**Expected Improvement:**
- Reduced ESP32 CPU usage in serialization
- Lower network bandwidth
- Faster parsing on client

**Implementation Priority:** Medium (Option A recommended for v2.1)

---

#### Issue 2.2: sendLEDUpdate() Frequency
**Problem:** Fixed 20 FPS (50ms) regardless of effect type

**Issue:** Static effects (solid colors) don't need 20 FPS updates

**Solution:** Adaptive update rate based on effect

**Adaptive FPS Strategy:**
```cpp
// Effect → Min FPS mapping
const struct {
  const char* effect;
  uint8_t minFPS;
} effectFPS[] = {
  {"Fire", 10},
  {"RainbowBeat", 15},
  {"MovingDot", 20},
  {"Comet", 15},
  {"Breath", 10},
  {"SolidColor", 1},  // Only update on change
  // ...
};
```

**Expected Improvement:**
- 50% reduction for static effects
- Smoother animation for fast effects
- Lower ESP32 CPU usage

**Implementation Priority:** High

---

### 3. Asset Loading & Compression

#### Issue 3.1: No Asset Minification
**Problem:** CSS/JS files served unminified

**Current Sizes:**
- wled-theme.css: 2.3KB
- sml-custom.css: 5.3KB
- responsive-nav.css: 4.3KB
- peek-preview.css: 4.1KB
- effects-config.css: 4.1KB
- config-tab.css: ~8KB
- **Total CSS:** ~28KB

- tabs-manager.js: 6.6KB
- effects-handler.js: ~19KB
- peek-render.js: 9.3KB
- config-manager.js: ~12KB
- main.js: 11.7KB
- **Total JS:** ~58KB

**Solution:** Minify for production

**Tools:**
- CSS: csso or clean-css
- JS: terser or uglify-js

**Expected Reduction:**
- CSS: 28KB → ~18KB (36% reduction)
- JS: 58KB → ~38KB (34% reduction)

**Implementation Priority:** Medium

---

#### Issue 3.2: No Compression Enabled
**Problem:** ESP32 serves files without gzip compression

**Solution:** Enable gzip compression in AsyncWebServer

**Implementation:**
```cpp
// In server setup
server.serveStatic("/", LittleFS, "/")
  .setDefaultFile("index.html")
  .setFilter([](AsyncWebServerRequest *request) {
    return true; // Enable gzip
  })
  .setCacheControl("max-age=3600");
```

**Expected Improvement:**
- HTML: ~15KB → ~4KB (73% reduction)
- CSS: ~28KB → ~7KB (75% reduction)
- JS: ~58KB → ~15KB (74% reduction)

**Implementation Priority:** High

---

#### Issue 3.3: FontAwesome Icon Loading
**Problem:** All FontAwesome icons loaded even if unused

**Current:** solid.css (~100KB)

**Solution:** Use FontAwesome subset or SVG sprites

**Option A: Subset Icons**
- Extract only used icons (fas: lightbulb, music, eye, thermometer, battery, cog, etc.)
- **Reduction:** 100KB → ~10KB

**Option B: Inline SVG**
- Replace critical icons with inline SVG
- **Reduction:** Zero external font load

**Implementation Priority:** Medium

---

### 4. JavaScript Execution

#### Issue 4.1: No Code Splitting
**Problem:** All JavaScript loaded upfront, even unused modules

**Solution:** Dynamic imports for non-critical modules

**Example:**
```javascript
// Load config-manager.js only when Config tab accessed
document.querySelector('[data-tab="config"]').addEventListener('click', async () => {
  await import('/js/config-manager.js');
});
```

**Expected Improvement:**
- Faster initial page load
- Lower memory footprint
- Parse less unused code

**Implementation Priority:** Low (nice-to-have for v2.2)

---

#### Issue 4.2: Excessive Console Logging
**Problem:** Debug logging in production

**Solution:** Strip console.log() in production build

**Tool:** terser with `drop_console: true`

**Expected Improvement:** Minimal (but cleaner code)

**Implementation Priority:** Low

---

### 5. Mobile-Specific Optimizations

#### Issue 5.1: No Touch Action Optimization
**Problem:** Default touch behavior causes 300ms delay

**Solution:** CSS touch-action property

```css
button, .nav-item, .sidebar-item {
  touch-action: manipulation; /* Disable double-tap zoom */
}
```

**Expected Improvement:** Instant touch response

**Implementation Priority:** High

---

#### Issue 5.2: Large Viewport on Mobile
**Problem:** No viewport meta tag optimization

**Current:**
```html
<meta name="viewport" content="width=device-width, initial-scale=1.0">
```

**Optimized:**
```html
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
```

**Expected Improvement:** No accidental zoom, consistent layout

**Implementation Priority:** High

---

#### Issue 5.3: No Passive Event Listeners
**Problem:** Scroll listeners block main thread

**Solution:** Mark scroll/touch listeners as passive

**Example:**
```javascript
window.addEventListener('scroll', handler, { passive: true });
```

**Expected Improvement:** Smoother scrolling

**Implementation Priority:** Medium

---

### 6. ESP32 Optimizations

#### Issue 6.1: Stack Size Monitoring
**Problem:** Unknown actual stack usage

**Solution:** Add detailed stack tracking

**Implementation:**
```cpp
#ifdef DEBUG_WEBSOCKET
debugD("WebSocket stack: ");
debugD_NUM(uxTaskGetStackHighWaterMark(NULL), "%u");
debuglnD(" bytes free");
#endif
```

**Expected Improvement:** Better visibility into stack usage

**Implementation Priority:** Low (debug only)

---

#### Issue 6.2: ArduinoJson Memory Pool
**Problem:** Dynamic allocation for every JSON message

**Current:** `JsonDocument json;` (auto-sizing)

**Solution:** Static allocation with fixed size

**Optimization:**
```cpp
StaticJsonDocument<2048> jsonDoc;  // Pre-allocated
JsonDocument json = jsonDoc;       // Use as before
```

**Expected Improvement:** No heap fragmentation

**Implementation Priority:** Medium

---

## Implementation Priority Summary

### Phase 1: Critical (Do Now)
1. ✅ Touch-action optimization (mobile)
2. ✅ Viewport meta tag fix
3. ✅ Adaptive LED update frequency
4. ✅ Enable gzip compression

**Expected Impact:** 30-40% overall performance improvement

### Phase 2: High (Do Soon)
1. Compact JSON format for LED data
2. Canvas gradient caching
3. FontAwesome subsetting
4. Asset minification

**Expected Impact:** Additional 20-30% improvement

### Phase 3: Medium (Nice to Have)
1. Object reuse in render loop
2. Static JSON document allocation
3. Passive event listeners
4. Code splitting

**Expected Impact:** 10-15% improvement

### Phase 4: Low (Future)
1. Binary LED protocol
2. Dirty rectangle tracking
3. Console log stripping

**Expected Impact:** 5-10% improvement

---

## Performance Testing Plan

### Before Optimization
1. Run Chrome DevTools Performance profiler
2. Record baseline metrics (FPS, memory, CPU)
3. Document ESP32 stack usage
4. Test on mobile devices

### After Optimization
1. Re-run same tests
2. Compare metrics
3. Verify no regressions
4. Document improvements

### Tools
- **Chrome DevTools:** Performance, Memory, Network tabs
- **Lighthouse:** Overall score, metrics
- **ESP32 Serial:** Stack monitoring, timing
- **Physical Devices:** Real-world testing

---

## Success Criteria

### Metrics to Achieve
- **Canvas FPS:** ≥30 FPS (desktop), ≥20 FPS (mobile)
- **WebSocket CPU:** <10% ESP32 CPU time
- **Page Load:** <2s First Contentful Paint
- **Memory:** Stable heap size (no leaks)
- **Mobile:** Instant touch response (<50ms)

### Regression Tests
- All existing functionality works
- No new console errors
- Cross-browser compatibility maintained
- ESP32 stability verified

---

## Implementation Timeline

**Week 1:** Phase 1 optimizations (critical)
**Week 2:** Phase 2 optimizations (high)
**Week 3:** Testing and validation
**Week 4:** Documentation and deployment

---

## Sign-Off

**Plan Created:** 2026-05-10
**Ready for Implementation:** ✅ Yes
**Estimated Effort:** 20-30 hours

**Next Step:** Begin Phase 1 optimizations
