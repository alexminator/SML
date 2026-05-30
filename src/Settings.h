// Settings.h - ARCHIVO ÚNICO DE CONFIGURACIÓN CENTRALIZADA
// Reemplaza completamente a Globals.h
#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include <FastLED.h>

// ============================================================================
// CONSTANTES HARDWARE
// ============================================================================

// LED Strip
#define STRIP_PIN 4
#define N_PIXELS 24
#define VOLTS 5
#define MAX_MILLIAMPS 500
#define COLOR_ORDER GRB
#define LED_TYPE WS2812B

// VU Meter
#define AUDIO_IN_PIN 36
#define DC_OFFSET 0
#define NOISE 30
#define SAMPLES 60
#define TOP (N_PIXELS + 2)
#define PEAK_FALL 20
#define N_PIXELS_HALF (N_PIXELS / 2)
#define BIAS 1850

// ============================================================================
// EFECTO BALLS
// ============================================================================
#define GRAVITY -1
#define h0 1
#define NUM_BALLS 3

// Variables Balls
uint8_t ballsCount = 3;
bool ballsRandomColors = false;
float h[NUM_BALLS];
float vImpact0;
float vImpact[NUM_BALLS];
float tCycle[NUM_BALLS];
int pos[NUM_BALLS];
long tLast[NUM_BALLS];
float COR[NUM_BALLS];

// ============================================================================
// EFECTO FIRE
// ============================================================================
uint8_t fireCooling = 55;
uint8_t fireSparking = 50;
bool fireReverse = false;

// ============================================================================
// EFECTO SINELON
// ============================================================================
uint8_t sinelonBeat = 23;
uint8_t sinelonFade = 2;

// ============================================================================
// EFECTO TWINKLE
// ============================================================================
uint8_t twinkleSpeed = 8;
uint8_t twinkleIntensity = 160;
bool twinkleRedCool = true;

// ============================================================================
// EFECTO RAINBOW BEAT
// ============================================================================
uint8_t rainbowSpeed = 30;
uint8_t rainbowDelta = 8;

// ============================================================================
// EFECTO MOVING DOT
// ============================================================================
uint8_t movingDotSpeed = 30;
uint8_t movingDotFade = 10;

// ============================================================================
// EFECTO RIPPLE
// ============================================================================
uint8_t rippleSize = 3;
bool rippleMirror = false;

// ============================================================================
// EFECTO COMET
// ============================================================================
uint8_t cometSpeed = 8;
uint8_t cometTrail = 4;
bool cometBlur = false;

// ============================================================================
// EFECTO BREATH
// ============================================================================
uint8_t breathSpeed = 128;

// ============================================================================
// EFECTO COLOR SWEEP
// ============================================================================
uint8_t sweepSpeed = 128;

// ============================================================================
// EFECTO JUGGLE
// ============================================================================
uint8_t juggleDots = 4;
uint8_t juggleSpeed = 64;
uint8_t juggleIntensity = 128;

// ============================================================================
// VARIABLES GLOBALES LEDS Y COLOR
// ============================================================================
CRGB leds[N_PIXELS];
uint8_t myhue = 0;
CRGBPalette16 currentPalette;
CRGBPalette16 targetPalette;

// ============================================================================
// VARIABLES VU METER
// ============================================================================
uint8_t volCount = 0;
int vol[SAMPLES];
int lvl = 0;
uint8_t volCountLeft = 0;
int volLeft[SAMPLES];
int lvlLeft = 0;
int minLvlAvgLeft = 0;
int maxLvlAvgLeft = 512;
bool is_centered = false;

// ============================================================================
// DECLARACIÓN DEL STRUCT STRIPLED (implementación en main.cpp)
// ============================================================================
struct StripLed {
    int R;
    int G;
    int B;
    int brightness;
    int effectId;
    bool powerState;

    StripLed();
    void simpleColor(int ar, int ag, int ab, int brightness);
    void update();
    void clear();
    void runFire();
    void runMovingDot();
    void runRainbowBeat();
    void runRedWhiteBlue();
    void runRipple();
    void runTwinkle();
    void runBalls();
    void runJuggle();
    void runSinelon();
    void runComet();
    void runBreath();
    void runColorSweep();
    void runRainbowVU();
    void runOldVU();
    void runRainbowHueVU();
    void runRippleVU();
    void runThreebarsVU();
    void runOceanVU();
    void runTemperature();
    void runBattery();
};

extern StripLed stripLed;

// ============================================================================
// DECLARACIONES DE FUNCIONES EXTERNAS (si son necesarias)
// ============================================================================
void readSensor();

#endif // SETTINGS_H