# WLED Effects Comparison - SML Project

## Effects Status and Required Improvements

### ✅ Comet - FIXED
- **Status**: Updated to WLED implementation
- **Parameters**: `cometSpeed=128` (0-255), `cometTrail=224` (0-255, fade rate)
- **WLED metadata**: `"Lighthouse@!,Fade rate;!,!;!;"`
- **Changes**: Uses time-based position calculation instead of delay

---

### 🔄 Twinkle - NEEDS UPDATE
**Current Implementation**:
```cpp
// Basic twinkle with fixed spawn rate
uint8_t twinkleDensity = 120;  // spawn rate
bool twinkleBlackBg = true;    // background type
bool twinkleSparkles = false;  // white sparkles
```

**WLED Implementation** (`mode_twinkle`):
- Uses PRNG16 for random number generation
- `SEGENV.aux0` = number of pixels to twinkle (controls density)
- Metadata: `"Twinkle@!,!;!,!;!;;m12=0"` (no speed/intensity in UI)
- Fade out all pixels, then randomly light up `aux0` pixels

**Required Changes**:
- Replace with WLED's `mode_twinkle` logic
- Change `twinkleDensity` (0-255) to control pixel count
- Remove `twinkleBlackBg` and `twinkleSparkles` (not in WLED)
- Use PRNG16 for better random distribution

---

### ✅ Fire 2012 - GOOD (minor update needed)
**Current Implementation**:
```cpp
uint8_t fireCooling = 55;   // cooling rate
uint8_t fireSparking = 50;  // spark probability
bool fireReverse = false;   // direction
```

**WLED Implementation** (`mode_fire_2012`):
- Metadata: `"Fire 2012@Cooling,Spark rate,,2D Blur,Boost;;!;1;pal=35,sx=64,ix=160,m12=1,c2=128"`
- Uses same cooling/sparking logic
- Additional: 2D blur, boost options

**Required Changes**:
- Update defaults to WLED values: `fireCooling=55`, `fireSparking=120`
- Add `fireBlur` parameter (optional, for 2D blur)
- Keep current implementation (already WLED-compatible)

---

### 🔄 Sinelon - NEEDS UPDATE
**Current Implementation**:
```cpp
uint8_t sinelonBeat = 23;  // beat frequency
uint8_t sinelonFade = 2;   // fade rate
```

**WLED Implementation** (`sinelon_base`):
- Metadata: `"Sinelon@!,Trail;!,!,!;!;"`
- Speed = `SEGMENT.speed/10` (beatsin16 divisor)
- Intensity = `SEGMENT.intensity` (fade amount via `fade_out()`)
- Dual mode option (sinelon_dual)

**Required Changes**:
- Change `sinelonBeat` to `sinelonSpeed` (0-255, default ~128)
- Divide speed by 10 in code: `beatsin16(sinelonSpeed/10, ...)`
- Keep `sinelonFade` as intensity parameter
- Add `sinelonDual` boolean for dual mode (optional)

---

### 🔄 Juggle - NEEDS UPDATE
**Current Implementation**:
```cpp
uint8_t juggleDots = 4;   // number of dots
uint8_t juggleFreq = 30;  // frequency
```

**WLED Implementation** (`mode_juggle`):
- Fixed 8 dots (not configurable)
- `fadeToBlackBy(192 - (3*SEGMENT.intensity/4))`
- Each dot uses `beatsin16(i*2, 0, SEGLEN-1)` for position

**Required Changes**:
- Remove `juggleFreq` parameter (WLED doesn't use it)
- Keep `juggleDots` but change default to 8 (WLED standard)
- Update beat calculation: `beatsin16(i*2, ...)` instead of frequency
- Adjust fade formula to WLED: `192 - (3*juggleDots/4)`

---

### ✅ Ripple - GOOD (minor update needed)
**Current Implementation**:
```cpp
uint8_t rippleSize = 3;     // wave size
bool rippleMirror = false;  // mirror mode
```

**WLED Implementation** (`mode_ripple`):
- Metadata: `"Ripple@!,Wave #,Blur,,,,Overlay;,!;!;12;c1=0"`
- Uses `SEGMENT.custom1>>1` for blur amount
- `SEGMENT.speed>>4` for decay rate
- Max ripples based on segment length

**Required Changes**:
- Add `rippleBlur` parameter (0-255, mapped to blur amount)
- Add `rippleDecay` parameter (uses speed in WLED)
- Keep current implementation (already good)

---

### ❓ Balls - CHECK WLED
**Current Implementation**:
```cpp
uint8_t ballsCount = 3;          // number of balls
bool ballsRandomColors = false;  // color mode
```

**WLED**: Has `mode_bouncing_balls` - need to check implementation

**Action Required**: Search for WLED bouncing balls implementation

---

## Summary of Changes Needed

### High Priority (Breaking Changes)
1. **Comet** ✅ - DONE (already updated)
2. **Sinelon** - Update speed calculation to WLED style
3. **Juggle** - Update to WLED's 8-dot standard

### Medium Priority (Parameter Updates)
4. **Twinkle** - Replace with WLED implementation
5. **Fire** - Update default values
6. **Ripple** - Add blur/decay parameters

### Low Priority (Optional Features)
7. **Balls** - Verify against WLED bouncing balls
8. Add dual mode options (sinelon_dual, etc.)

---

## Implementation Order

1. ✅ **Comet** - DONE
2. **Sinelon** - Quick fix (speed/10 formula)
3. **Juggle** - Update to 8 dots
4. **Twinkle** - Replace implementation
5. **Fire** - Update defaults
6. **Ripple** - Add blur parameter
7. **Balls** - Check WLED implementation
