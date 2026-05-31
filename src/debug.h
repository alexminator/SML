// Logging (from Serial.print statements) can be customised by setting the verbosity here:
/*
    This provides a superior form of debugging.

    * the function and line number of the debugging statement is output along with the message
    * the supporting debug code is now in a separate header file to avoid program clutter
    * allows granular levels of debugging - some for errors, others for warnings and other that only appear when you really need them to
    * all controlled with a single #define statement in your code

    E - errors that you really, really should see
    W - warnings that help you find out what's going wrong
    D - debugging messages that you only want to see when debugging
    V - verbose meaning everything is printed

    To use, just #include this header file in your standard Arduino Code and then
    define the level of logging you want by setting the DEBUGLEVEL to one of the values
    listed here.

    In your code, instead of using Serial.print and Serial.println statements use
    debugE / debuglnE, debugW / debuglnW, D, V (as you see the level of logging required)
    and only those messages will appear.

    debuglnE("This is an error debugging statement for when things fail");
    debugW("A warning message that doesn't include a new line");
    debuglnD("A general debugging level statement");
    debuglnV("A verbose message, usually reserved for nitty, gritty debugging);

    Example:

    You want Errors, Warnings and General Debugging output but not those marked as Verbose.
    #define DEBUGLEVEL DEBUGLEVEL_DEBUGGING
*/

#ifndef DEBUG_H
#define DEBUG_H

// User picks debugging level from this list
#define DEBUGLEVEL_ERRORS 1
#define DEBUGLEVEL_WARNINGS 2
#define DEBUGLEVEL_DEBUGGING 3
#define DEBUGLEVEL_VERBOSE 4
#define DEBUGLEVEL_NONE 0



// We want to debug everything! Overide this in your sketch
#ifndef DEBUGLEVEL
#define DEBUGLEVEL DEBUGLEVEL_VERBOSE
#warning Debug Level is set to default level. \
To change this, type in your sketch:   \
#define DEBUGLEVEL followed by your preferred level of debugging\
before the #include statement
#endif

void debugNothing(...)
{
    // This does nothing and will be zapped by the compiler
}

// By default we want a trace stamp output in the first instance
bool traceStampRequired = true;

char debugStr[128];  // Buffer for debug messages
bool foundNL = false;

// Helper function to copy string to debug buffer (SAFE version - no snprintf)
inline void copyToDebugStr(const char* src) {
    if (src == nullptr) {
        debugStr[0] = '\0';
        foundNL = false;
        return;
    }

    // Safe copy with explicit length limit
    size_t i = 0;
    while (i < sizeof(debugStr) - 1 && src[i] != '\0') {
        debugStr[i] = src[i];
        i++;
    }
    debugStr[i] = '\0';  // Always null terminate
    foundNL = (strstr(debugStr, "\n") != NULL);
}

// Helper function for String objects
inline void copyToDebugStr(const String& src) {
    // Safe copy with explicit length limit
    size_t i = 0;
    const char* cstr = src.c_str();

    while (i < sizeof(debugStr) - 1 && cstr[i] != '\0') {
        debugStr[i] = cstr[i];
        i++;
    }
    debugStr[i] = '\0';  // Always null terminate
    foundNL = (strstr(debugStr, "\n") != NULL);
}

// The tracestamp looks like [D][mainfunction:45]
#define traceStamp(x, y, z)                \
    if (traceStampRequired)                \
    {                                      \
        Serial.print("[");                 \
        Serial.print(x);                   \
        Serial.print("]");                 \
        Serial.print("[");                 \
        Serial.print(__FUNCTION__);        \
        Serial.print(":");                 \
        Serial.print(__LINE__);            \
        Serial.print("] ");                \
    }                                      \
    copyToDebugStr(y);                     \
    if (z || foundNL)                      \
        traceStampRequired = true;         \
    else                                   \
        traceStampRequired = false;

// This is a macro that turns simple (one string) Serial.println statements on and off
#if DEBUGLEVEL > 0
#define debugE(x)              \
    traceStamp("E", x, false); \
    Serial.print(x);
#define debuglnE(x)           \
    traceStamp("E", x, true); \
    Serial.println(x)
#else
#define debugE(x)   // Nothing to see here
#define debuglnE(x) // Or here
#endif

#if DEBUGLEVEL > 1
#define debugW(x)              \
    traceStamp("W", x, false); \
    Serial.print(x);
#define debuglnW(x)           \
    traceStamp("W", x, true); \
    Serial.println(x)
#else
#define debugW(x)   // Nothing to see here
#define debuglnW(x) // Or here
#endif

#if DEBUGLEVEL > 2
#define debugD(x)              \
    traceStamp("D", x, false); \
    Serial.print(x);
#define debuglnD(x)           \
    traceStamp("D", x, true); \
    Serial.println(x)
#else
#define debugD(x)   /* Nothing to see here */
#define debuglnD(x) // Or here
#endif

#if DEBUGLEVEL > 3
#define debugV(x)              \
    traceStamp("V", x, false); \
    Serial.print(x);
#define debuglnV(x)           \
    traceStamp("V", x, true); \
    Serial.println(x);
#else
#define debugV(x) debugNothing(x);
#define debuglnV(x) debugNothing(x);
#endif

// ── Safe numeric debug macros (no snprintf — avoids ESP32 crash) ──────────────

// Integer (uses itoa — safe for %d, %u values up to INT_MAX)
#define debugD_NUM(val, format) \
    do { \
        char _buf[32]; \
        itoa(val, _buf, 10); \
        debugD(_buf); \
    } while(0)

#define debuglnD_NUM(val, format) \
    do { \
        char _buf[32]; \
        itoa(val, _buf, 10); \
        debuglnD(_buf); \
    } while(0)

// Float with 1 decimal place (e.g. "23.5")
// Manual conversion — avoids snprintf %.1f which crashes ESP32
#define debugD_FLOAT1(val) \
    do { \
        char _buf[32]; \
        int _int = (int)(val); \
        int _frac = (int)(((val) - _int) * 10); \
        if (_frac < 0) _frac = -_frac; \
        itoa(_int, _buf, 10); \
        size_t _flen = strlen(_buf); \
        _buf[_flen] = '.'; \
        _buf[_flen + 1] = '0' + _frac; \
        _buf[_flen + 2] = '\0'; \
        debugD(_buf); \
    } while(0)

#define debuglnD_FLOAT1(val) \
    do { \
        char _buf[32]; \
        int _int = (int)(val); \
        int _frac = (int)(((val) - _int) * 10); \
        if (_frac < 0) _frac = -_frac; \
        itoa(_int, _buf, 10); \
        size_t _flen = strlen(_buf); \
        _buf[_flen] = '.'; \
        _buf[_flen + 1] = '0' + _frac; \
        _buf[_flen + 2] = '\0'; \
        debuglnD(_buf); \
    } while(0)

// Zero-padded 3-digit number (e.g. "007")
#define debugD_NUM03(val) \
    do { \
        char _buf[32]; \
        int _v = (int)(val); \
        _buf[0] = '0' + (_v / 100) % 10; \
        _buf[1] = '0' + (_v / 10) % 10; \
        _buf[2] = '0' + _v % 10; \
        _buf[3] = '\0'; \
        debugD(_buf); \
    } while(0)

#endif  // DEBUG_H
