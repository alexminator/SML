/**
 * ----------------------------------------------------------------------------
 * EFFECT BASE CLASS
 * ----------------------------------------------------------------------------
 * Base class for all WLED-style effects with unified parameter system
 * ----------------------------------------------------------------------------
 */

#pragma once

#include <FastLED.h>
#include <string.h>
#include <stdlib.h>

// Default parameter values
#define DEFAULT_SPEED 128
#define DEFAULT_INTENSITY 128
#define DEFAULT_C1 128
#define DEFAULT_C2 128
#define DEFAULT_C3 16

/**
 * Unified parameter structure for all effects
 * Based on WLED's parameter system
 */
struct EffectParams {
    uint8_t speed;       // Effect speed (0-255)
    uint8_t intensity;   // Effect intensity (0-255)
    uint8_t custom1;     // Custom parameter 1 (0-255)
    uint8_t custom2;     // Custom parameter 2 (0-255)
    uint8_t custom3;     // Custom parameter 3 (0-255)
    bool check1;         // Boolean check 1 (true/false)
    bool check2;         // Boolean check 2 (true/false)
    bool check3;         // Boolean check 3 (true/false)
};

/**
 * Base Effect class
 * All LED effects inherit from this class
 */
class Effect {
protected:
    EffectParams params;  // Effect parameters
    CRGB* leds;           // Pointer to LED array
    uint16_t numLeds;     // Number of LEDs

public:
    /**
     * Constructor - initializes all parameters to defaults
     * @param ledArray Pointer to CRGB LED array
     * @param numLed Number of LEDs in the array
     */
    Effect(CRGB* ledArray, uint16_t numLed)
        : leds(ledArray), numLeds(numLed) {

        // Initialize all parameters to defaults
        params.speed = DEFAULT_SPEED;
        params.intensity = DEFAULT_INTENSITY;
        params.custom1 = DEFAULT_C1;
        params.custom2 = DEFAULT_C2;
        params.custom3 = DEFAULT_C3;
        params.check1 = false;
        params.check2 = false;
        params.check3 = false;
    }

    /**
     * Virtual destructor for proper cleanup in derived classes
     */
    virtual ~Effect() {}

    /**
     * Pure virtual render method - must be implemented by derived classes
     * This is where the effect's visual logic is implemented
     */
    virtual void render() = 0;

    /**
     * Run the effect - calls render() method
     * This provides a consistent interface for executing effects
     */
    void run() {
        render();
    }

    /**
     * Get metadata string for this effect (WLED-style format)
     * Format: "Name@label_speed,label_intensity,label_c1,...,label_c3,,label_m1,label_m2,label_m3;...;...;...;sx=64,ix=128,c1=55,c2=50,c3=16,m1=0,m2=0,m3=0"
     * Returns nullptr if effect has no metadata (no parameters to configure).
     */
    virtual const char* getMeta() const {
        return nullptr;
    }

    /**
     * Parse WLED-style metadata defaults and apply them to params.
     * Extracts sx=, ix=, c1=, c2=, c3=, m1=, m2=, m3= from the defaults
     * section (after the last ';') and sets corresponding params.
     * Resets all params to global defaults first, then overrides with meta values.
     */
    void setToDefaults(const char* metaStr) {
        // Reset to global defaults first
        params.speed     = DEFAULT_SPEED;
        params.intensity = DEFAULT_INTENSITY;
        params.custom1   = DEFAULT_C1;
        params.custom2   = DEFAULT_C2;
        params.custom3   = DEFAULT_C3;
        params.check1    = false;
        params.check2    = false;
        params.check3    = false;

        if (!metaStr) return;

        // Find last semicolon — defaults section
        const char* defs = strrchr(metaStr, ';');
        if (!defs || *(defs+1) == '\0') return;
        defs++;

        // Parse key=value pairs (comma or space separated)
        while (*defs) {
            // Skip whitespace and commas
            while (*defs == ' ' || *defs == ',') { defs++; }
            if (!*defs) break;

            if      (strncmp(defs, "sx=", 3) == 0) { params.speed     = (uint8_t)atoi(defs + 3); }
            else if (strncmp(defs, "ix=", 3) == 0) { params.intensity = (uint8_t)atoi(defs + 3); }
            else if (strncmp(defs, "c1=", 3) == 0) { params.custom1   = (uint8_t)atoi(defs + 3); }
            else if (strncmp(defs, "c2=", 3) == 0) { params.custom2   = (uint8_t)atoi(defs + 3); }
            else if (strncmp(defs, "c3=", 3) == 0) { params.custom3   = (uint8_t)atoi(defs + 3); }
            else if (strncmp(defs, "m1=", 3) == 0) { params.check1    = atoi(defs + 3) != 0; }
            else if (strncmp(defs, "m2=", 3) == 0) { params.check2    = atoi(defs + 3) != 0; }
            else if (strncmp(defs, "m3=", 3) == 0) { params.check3    = atoi(defs + 3) != 0; }

            // Advance past the value (until next comma or whitespace)
            while (*defs && *defs != ',' && *defs != ' ') defs++;
        }
    }

    // ========== Setter Methods ==========

    /**
     * Set effect speed
     * @param value Speed value (0-255)
     */
    void setSpeed(uint8_t value) {
        params.speed = value;
    }

    /**
     * Set effect intensity
     * @param value Intensity value (0-255)
     */
    void setIntensity(uint8_t value) {
        params.intensity = value;
    }

    /**
     * Set custom parameter 1
     * @param value Custom 1 value (0-255)
     */
    void setCustom1(uint8_t value) {
        params.custom1 = value;
    }

    /**
     * Set custom parameter 2
     * @param value Custom 2 value (0-255)
     */
    void setCustom2(uint8_t value) {
        params.custom2 = value;
    }

    /**
     * Set custom parameter 3
     * @param value Custom 3 value (0-255)
     */
    void setCustom3(uint8_t value) {
        params.custom3 = value;
    }

    /**
     * Set check 1 (boolean)
     * @param value Boolean value
     */
    void setCheck1(bool value) {
        params.check1 = value;
    }

    /**
     * Set check 2 (boolean)
     * @param value Boolean value
     */
    void setCheck2(bool value) {
        params.check2 = value;
    }

    /**
     * Set check 3 (boolean)
     * @param value Boolean value
     */
    void setCheck3(bool value) {
        params.check3 = value;
    }

    // ========== Getter Methods ==========

    /**
     * Get current speed value
     * @return Current speed (0-255)
     */
    uint8_t getSpeed() const {
        return params.speed;
    }

    /**
     * Get current intensity value
     * @return Current intensity (0-255)
     */
    uint8_t getIntensity() const {
        return params.intensity;
    }

    /**
     * Get custom parameter 1
     * @return Custom 1 value (0-255)
     */
    uint8_t getCustom1() const {
        return params.custom1;
    }

    /**
     * Get custom parameter 2
     * @return Custom 2 value (0-255)
     */
    uint8_t getCustom2() const {
        return params.custom2;
    }

    /**
     * Get custom parameter 3
     * @return Custom 3 value (0-255)
     */
    uint8_t getCustom3() const {
        return params.custom3;
    }

    /**
     * Get check 1 state
     * @return Boolean value
     */
    bool getCheck1() const {
        return params.check1;
    }

    /**
     * Get check 2 state
     * @return Boolean value
     */
    bool getCheck2() const {
        return params.check2;
    }

    /**
     * Get check 3 state
     * @return Boolean value
     */
    bool getCheck3() const {
        return params.check3;
    }
};
