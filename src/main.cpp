// main.cpp
/**
 * ----------------------------------------------------------------------------
 * SMART MUSIC LAMP
 * ----------------------------------------------------------------------------
 * © 2023 Alexminator
 * ----------------------------------------------------------------------------
 */

#include <Arduino.h>
#include <FastLED.h>
#include "state/AppState.h"
#include "effects/EffectRegistry.h"
#include "net/WebSocket.h"
#include "net/WebServer.h"
#include "tasks/tasks.h"
#include "config/debug_config.h"

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

    initLittleFS();
    loadEffectParams();
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