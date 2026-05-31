// main.cpp
/**
 * ----------------------------------------------------------------------------
 * SMART MUSIC LAMP
 * ----------------------------------------------------------------------------
 * © 2023 Alexminator
 * ----------------------------------------------------------------------------
 */

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <Battery18650Stats.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Preferences.h>
#include "config/secrets.h"

// ── WIFI credentials — definiciones reales (data.h tiene solo extern) ──────────
const char *WIFI_SSID = DEFAULT_WIFI_SSID;
const char *WIFI_PASS = DEFAULT_WIFI_PASS;
const char *WEB_NAME = "sml";

#include "data.h"
#include "Settings.h"
#include "net/WebSocket.h"
#include "net/WebServer.h"
#include "power/PowerMgr.h"
#include "tasks/tasks.h"

// Declare the debugging level then include the header file
#define DEBUGLEVEL DEBUGLEVEL_DEBUGGING
// #define DEBUGLEVEL DEBUGLEVEL_NONE
#include "debug.h"

// ============================================================================
// DEBUG CATEGORIES - Uncomment to enable specific debug messages
// ============================================================================
// System-level messages (initialization, mutex, errors)
#define DEBUG_SYSTEM

// WiFi connection and authentication
#define DEBUG_WIFI

// Web server and LittleFS filesystem
// #define DEBUG_WEB

// WebSocket communication and JSON
// #define DEBUG_WEBSOCKET

// LED strip control and effects
// #define DEBUG_LED

// Battery monitoring and charging
// #define DEBUG_BATTERY

// Temperature and humidity sensor (DHT)
// #define DEBUG_TEMPERATURE

// Network services (mDNS)
#define DEBUG_NETWORK

// BLUETOOTH
// #define DEBUG_BLUETOOTH

// Power Management State Machine debugging
#define DEBUG_POWER_MANAGEMENT

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

// server definido en net/WebServer.cpp

// ----------------------------------------------------------------------------
// RTOS Mutex Protection
// ----------------------------------------------------------------------------
// dataMutex y wifiMutex definidos en state/AppState.cpp

void initMutexes() {
    dataMutex = xSemaphoreCreateMutex();
    wifiMutex = xSemaphoreCreateMutex();

    if (dataMutex == NULL || wifiMutex == NULL) {
#ifdef DEBUG_SYSTEM
        debuglnE("Failed to create mutexes!");
        debuglnE("System may experience race conditions");
#endif
    } else {
#ifdef DEBUG_SYSTEM
        debuglnD("Mutexes initialized successfully");
#endif
    }
}

// ============================================================================
// DEFINICIÓN DE MÉTODOS DEL STRUCT BATTERY
// ============================================================================

void Battery::battMonitor() {
    int isCharging = digitalRead(CHARGE_PIN);
    chargeState = isCharging == LOW;
    int fullyCharge = digitalRead(FULL_CHARGE_PIN);
    fullBatt = fullyCharge == LOW;
    battVolts = batteryStats.getBatteryVolts();
    battLvl = batteryStats.getBatteryChargeLevel(true);

#ifdef DEBUG_BATTERY
    debugD(chargeState ? "Cargador conectado" : "Cargador desconectado");
    debugD("\n");
    debugD("Estado del pin carga: ");
    debugD(fullyCharge ? "LOW (charging)" : "HIGH (full/not charging)");
    debugD("\n");
    if (!fullBatt && !chargeState) {
        debuglnD("Batería usándose");
    } else if (fullBatt) {
        debuglnD("Batería completamente cargada");
    } else {
        debuglnD("Batería cargándose");
    }
    debugD("Lectura promedio: ");
    debugD_NUM(batteryStats.pinRead(), "%d");
    debugD(", Voltaje: ");
    debugD_NUM((int)(battVolts * 1000), "%d");
    debugD(".");
    debugD_NUM03((int)((battVolts * 1000) % 1000));
    debugD(", Nivel: ");
    debugD_NUM(battLvl, "%d");
    debuglnD("%");
#endif
}

// ============================================================================
// DEFINICIÓN DEL STRUCT STRIPLED (MÉTODOS)
// ============================================================================
// stripLed, onboard_led, batt — definidos en state/AppState.cpp

StripLed::StripLed() : R(255), G(255), B(255), brightness(130), effectId(0), powerState(false) {}  // Inicializado con el color blanco el brillo en la mitad y efecto 0

void StripLed::simpleColor(int ar, int ag, int ab, int brightness) {
    for (int i = 0; i < N_PIXELS; i++) {
        leds[i] = CRGB(ar, ag, ab);
    }
    FastLED.setBrightness(brightness);
    FastLED.show();
}

void StripLed::clear() {
    FastLED.clear();
    FastLED.show();
}

// ============================================================================
// EFFECTS LIBRARY INCLUDES
// ============================================================================
#include "common.h"
#include "MovingDot.h"
#include "RainbowBeat.h"
#include "RedWhiteBlue.h"
#include "Ripple.h"
#include "Fire.h"
#include "Twinkle.h"
#include "Balls.h"
#include "Juggle.h"
#include "Sinelon.h"
#include "Comet.h"
#include "Temp.h"
#include "Breath.h"
#include "ColorSweep.h"
#include "Battery.h"
// VU
#include "vu1.h"
#include "vu2.h"
#include "vu3.h"
#include "vu4.h"
#include "vu5.h"
#include "vu6.h"

void StripLed::runFire() {
    static Fire fire;
    fire.runPattern();
}

void StripLed::runMovingDot() {
    static MovingDot movingDot;
    movingDot.runPattern();
}

void StripLed::runRainbowBeat() {
    static RainbowBeat rainbowBeat;
    rainbowBeat.runPattern();
}

void StripLed::runRedWhiteBlue() {
    static RedWhiteBlue redWhiteBlue;
    redWhiteBlue.runPattern();
}

void StripLed::runRipple() {
    static Ripple ripple;
    ripple.runPattern();
}

void StripLed::runTwinkle() {
    static Twinkle twinkle;
    twinkle.runPattern();
}

void StripLed::runBalls() {
    static Balls balls;
    balls.runPattern();
}

void StripLed::runJuggle() {
    static Juggle juggle;
    juggle.runPattern();
}

void StripLed::runSinelon() {
    static Sinelon sinelon;
    sinelon.runPattern();
}

void StripLed::runComet() {
    static Comet comet;
    comet.runPattern();
}

void StripLed::runBreath() {
    static Breath breath;
    breath.runPattern();
}

void StripLed::runColorSweep() {
    static ColorSweep colorSweep;
    colorSweep.runPattern();
}

void StripLed::runRainbowVU() {
    static RainbowVU VU1;
    VU1.runPattern(is_centered, 0);
}

void StripLed::runOldVU() {
    static OldskoolVU VU2;
    VU2.runPattern(is_centered, 0);
}

void StripLed::runRainbowHueVU() {
    static RainbowHueVU VU3;
    VU3.runPattern(is_centered, 0);
}

void StripLed::runRippleVU() {
    static RippleVU VU4;
    VU4.runPattern(true);
}

void StripLed::runThreebarsVU() {
    static ThreebarsVU VU5;
    VU5.runPattern();
}

void StripLed::runOceanVU() {
    static OceanVU VU6;
    VU6.runPattern();
}

void StripLed::runTemperature() {
    static Temperature temp;
    temp.runPattern();
}

void StripLed::runBattery() {
    static Charge batt;
    batt.runPattern(lvlCharge);
}

void StripLed::update() {
    if (!powerState) {
        clear();
        return;
    }
    if (effectId == 0) {
        simpleColor(R, G, B, brightness);
        return;
    }
    clear();
    runEffectById(effectId);
}

// readSensor() → tasks/tasks.cpp
// SPIFFS initialization
// ----------------------------------------------------------------------------

// initLittleFS(), initWiFi(), initWebServer(), processor(), onRootRequest()
// → extraídos a net/WebServer.h / net/WebServer.cpp

// notifyClients(), handleWebSocketMessage(), onWsEvent(), initWebSocket()
// bars() → extraídos a net/WebSocket.h / net/WebSocket.cpp

// Task functions → tasks/tasks.h / tasks/tasks.cpp

// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

void setup()
{
    delay(3000); // sanity delay
    pinMode(onboard_led.pin, OUTPUT);
    pinMode(STRIP_PIN, OUTPUT);
    pinMode(LAMP_PIN, OUTPUT);
    pinMode(SWITCH_PIN, OUTPUT);
    pinMode(CHARGE_PIN, INPUT);
    pinMode(ADC_PIN, INPUT);
    pinMode(FULL_CHARGE_PIN, INPUT);
    pinMode(VOLUMENUP_PIN, OUTPUT);
    pinMode(VOLUMENDOWN_PIN, OUTPUT);
    pinMode(PLAY_PIN, OUTPUT);

    // Init Rele on OFF
    digitalWrite(LAMP_PIN, HIGH);
    digitalWrite(SWITCH_PIN, HIGH);

    // Init Mosfet on OFF. Emulate Button not pressed
    digitalWrite(VOLUMENUP_PIN, LOW);
    digitalWrite(VOLUMENDOWN_PIN, LOW);
    digitalWrite(PLAY_PIN, LOW);

    Serial.begin(115200);
    initMutexes();

    // Initialize DHT22 device.
    dht.begin();
    // Print temperature sensor details.
    sensor_t sensor;
    dht.temperature().getSensor(&sensor);
#ifdef DEBUG_TEMPERATURE
    debuglnD("Temperature Sensor");
    debugD("Sensor Type: ");
    debuglnD(sensor.name);
    debugD("Driver Ver: ");
    debugD_NUM(sensor.version, "%d");
    debugD("\n");
    debugD("Unique ID: ");
    debugD_NUM(sensor.sensor_id, "%u");
    debugD("\n");
    debugD("Max Value: ");
    debugD_FLOAT1(sensor.max_value);
    debugD("°C\n");
    debugD("Min Value: ");
    debugD_FLOAT1(sensor.min_value);
    debugD("°C\n");
    debugD("Resolution: ");
    debugD_FLOAT1(sensor.resolution);
    debugD("°C\n");
    debuglnD("----------------------------------------------------");
#endif
    // Print humidity sensor details.
    dht.humidity().getSensor(&sensor);
#ifdef DEBUG_TEMPERATURE
    debuglnD("Humidity Sensor");
    debugD("Sensor Type: ");
    debuglnD(sensor.name);
    debugD("Driver Ver: ");
    debugD_NUM(sensor.version, "%d");
    debugD("\n");
    debugD("Unique ID: ");
    debugD_NUM(sensor.sensor_id, "%u");
    debugD("\n");
    debugD("Max Value: ");
    debugD_FLOAT1(sensor.max_value);
    debugD("%\n");
    debugD("Min Value: ");
    debugD_FLOAT1(sensor.min_value);
    debugD("%\n");
    debugD("Resolution: ");
    debugD_FLOAT1(sensor.resolution);
    debugD("%\n");
    debuglnD("----------------------------------------------------");
#endif
    
    // For FASTLED library
    FastLED.addLeds<LED_TYPE, STRIP_PIN, COLOR_ORDER>(leds, N_PIXELS).setCorrection(TypicalLEDStrip);
    FastLED.setMaxPowerInVoltsAndMilliamps(VOLTS, MAX_MILLIAMPS);
    FastLED.setBrightness(stripLed.brightness);
    // Clear all neo's
    FastLED.clear();
    FastLED.show();

    // Initialize variables for balls effect
    for (int i = 0; i < NUM_BALLS; i++)
    {
        tLast[i] = millis();
        h[i] = h0;
        pos[i] = 0;            // Balls start on the ground
        vImpact[i] = vImpact0; // And "pop" up at vImpact0
        tCycle[i] = 0;
        COR[i] = 0.90 - float(i) / pow(NUM_BALLS, 2);
    }

    initLittleFS();
    initWiFi();
    initWebSocket();
    initWebServer();

    initTasks();
}

// ----------------------------------------------------------------------------
// Main control loop
// ----------------------------------------------------------------------------

void loop()
{

}