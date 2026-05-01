# Critical Bugs Fix - Testing Checklist

**Date:** 2026-04-16
**Branch:** `bugfix/critical-bugs`
**Commits:** 4 new commits (plus #5 already in master)

---

## Implementation Summary

✅ **Bug #5: Infinite Loop in WiFi Init** - Replaced with 30-second timeout
✅ **Bug #1: WiFi Credentials to Build-Time** - Moved to .env file
✅ **Bug #2: WebSocket Buffer Overflow** - Dynamic buffer sizing
✅ **Bug #4: Race Conditions** - Mutex protection added
✅ **Bug #3: Memory Fragmentation** - String → char arrays

---

## Pre-Testing Setup

### 1. Configure Environment
```bash
# Copy and configure .env file
cp .env.example .env
# Edit .env with your WiFi credentials
```

### 2. Build and Flash
```bash
# Build project
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output
pio device monitor
```

---

## Individual Bug Testing

### Bug #5: Infinite Loop Timeout
- [ ] Normal boot: LittleFS mounts successfully
- [ ] Error handling: Corrupt LittleFS, system continues after 30s
- [ ] LED pattern: Flashes 50ms on/150ms off during error
- [ ] Debug messages: "LittleFS mount failed" appears

### Bug #1: WiFi Credentials
- [ ] Build succeeds with .env credentials
- [ ] Device connects to WiFi using .env credentials
- [ ] Web interface `/save-wifi` still works
- [ ] No credentials visible in source code
- [ ] Debug messages show which credentials used

### Bug #2: WebSocket Buffer
- [ ] WebSocket connects without errors
- [ ] JSON messages sent correctly
- [ ] Debug shows: "WebSocket payload size: XXX bytes"
- [ ] No crashes during normal operation
- [ ] Stack monitoring warnings appear if low

### Bug #4: Race Conditions
- [ ] LED control smooth (no flicker)
- [ ] Battery level updates consistently
- [ ] WebSocket updates show consistent data
- [ ] WiFi reconnects after router power cycle
- [ ] No mutex error messages
- [ ] No data corruption

### Bug #3: Memory Fragmentation
- [ ] All debug messages work correctly
- [ ] WiFi connection/reconnection works
- [ ] Heap free remains stable (monitor for 30+ min)
- [ ] No crashes or resets
- [ ] Memory fragmentation < 10%

---

## Integration Testing

### 1-Hour Stress Test
- [ ] Run device for 1 hour continuous
- [ ] Monitor: No crashes, no resets
- [ ] Use all web interface controls
- [ ] Play music via Bluetooth
- [ ] Monitor all LED effects

### Functionality Verification
- [ ] LED color picker works
- [ ] All 18 LED effects work
- [ ] Brightness slider works
- [ ] Lamp on/off works
- [ ] Bluetooth controls work (play/pause, volume, skip)
- [ ] Battery monitoring displays correctly
- [ ] Temperature/humidity display correctly
- [ ] WebSocket updates work
- [ ] All web interface controls functional

### Performance Testing
- [ ] Stack usage < 80% in all tasks
- [ ] Heap fragmentation < 10%
- [ ] Heap free remains stable
- [ ] No memory leaks
- [ ] WiFi reconnection works

---

## Success Criteria

- [ ] ✅ Compiles with 0 errors, 0 warnings
- [ ] ✅ All existing functionality 100% operational
- [ ] ✅ System runs 1+ hour without crashes
- [ ] ✅ Heap fragmentation < 10%
- [ ] ✅ Stack usage < 80% in all tasks
- [ ] ✅ WiFi reconnection working
- [ ] ✅ No mutex errors or race conditions
- [ ] ✅ WebSocket buffer overflow prevented
- [ ] ✅ LittleFS failure no longer hangs system
- [ ] ✅ No credentials exposed in source code

---

## Known Changes

### Modified Files
- `src/main.cpp` - All bug fixes
- `src/data.h` - WiFi credential handling
- `src/debug.h` - String → char array
- `platformio.ini` - Build flags for credentials
- `.gitignore` - Added .env
- `.env` - New file (not committed)

### No Changes Required
- All LED effect files (no changes)
- Web interface files (no visual changes)
- Hardware configuration (no changes)

---

## Post-Testing Actions

### If All Tests Pass
1. Merge to master: `git checkout master && git merge bugfix/critical-bugs`
2. Push to origin: `git push origin master`
3. Delete branch: `git branch -d bugfix/critical-bugs`
4. Tag release: `git tag -a v1.1.0 -m "Critical bugs fix"`

### If Issues Found
1. Document issues found
2. Create bug reports
3. Determine if rollback needed
4. Plan additional fixes

---

## Monitoring Code (Optional)

For detailed testing, temporarily add to `TaskOnboardLED()`:

```cpp
// Every 60 seconds, print heap stats
static uint32_t lastHeapPrint = 0;
if (millis() - lastHeapPrint > 60000) {
    debugD("Free heap: ");
    debugD_NUM(ESP.getFreeHeap(), "%u");
    debugD(" bytes\n");
    lastHeapPrint = millis();
}
```

Remove this monitoring code after testing complete.

---

**END OF TESTING CHECKLIST**
