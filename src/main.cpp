/**
 * ----------------------------------------------------------------------------
 * SMART MUSIC LAMP
 * ----------------------------------------------------------------------------
 * © 2023 Alexminator
 * ----------------------------------------------------------------------------
 */

#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <FastLED.h>
#include "data.h"

// ----------------------------------------------------------------------------
// Definition of macros
// ----------------------------------------------------------------------------
// Strip LED
#define STRIP_PIN 4
#define N_PIXELS 24
#define VOLTS 5           // Vcc Strip [5 volts]
#define MAX_MILLIAMPS 500 // Maximum current to draw [500 mA]
#define COLOR_ORDER GRB   // Colour order of LED strip [GRB]
#define LED_TYPE WS2812B  // LED string type [WS2812B]
//VU
# define AUDIO_IN_PIN 34             // Left aux in signal [A5]
# define DC_OFFSET 0                // DC offset in aux signal [0]
# define NOISE 20                   // Noise/hum/interference in aux signal [10]
# define SAMPLES 60                 // Length of buffer for dynamic level adjustment [60]
# define TOP (N_PIXELS + 2)         // Allow dot to go slightly off scale [(N_PIXELS + 2)]
# define PEAK_FALL 20               // Rate of peak falling dot [20]
# define N_PIXELS_HALF (N_PIXELS / 2)
# define PATTERN_TIME 10            // Seconds to show eaach pattern on auto [10]

uint8_t volCount = 0;           // Frame counter for storing past volume data
int vol[SAMPLES];               // Collection of prior volume samples
int lvl = 0;                    // Current "dampened" audio level
int minLvlAvg = 0;              // For dynamic adjustment of graph low & high
int maxLvlAvg = 512;

// WEB
#define HTTP_PORT 80

// Effects
#define GRAVITY -1  // Downward (negative) acceleration of gravity in m/s^2
#define h0 1        // Starting height, in meters, of the ball (strip length)
#define NUM_BALLS 3 // Number of bouncing balls you want (recommend < 7, but 20 is fun in its own way)

#if defined(ESP32)

#include <WiFi.h>
#include <ESPmDNS.h>
#include <Update.h>

#elif defined(ESP8266)

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#else
#error "Board not found"
#endif

AsyncWebServer server(HTTP_PORT);
AsyncWebSocket ws("/ws");

// ----------------------------------------------------------------------------
// Definition of global constants
// ----------------------------------------------------------------------------

//CMOS Switch for Bluetooth
#define SWITCH_PIN 5            // Pin to command swith MOSFET
bool bt_powerState = false;

// Web signal info
unsigned long startMillis;
unsigned long currentMillis;
const unsigned long refresh = 5000; // 5 seg
String strength;

// Strip LED
int brightness = 130;
int effectId = 0;
CRGB leds[N_PIXELS];
uint8_t myhue = 0;               // hue 0. red color
const uint8_t FADE_RATE = 2; // How long should the trails be. Very low value = longer trails.
uint8_t r = 255;
uint8_t g = 255;
uint8_t b = 255;

// balls effect
float h[NUM_BALLS];                       // An array of heights
float vImpact0 = sqrt(-2 * GRAVITY * h0); // Impact velocity of the ball when it hits the ground if "dropped" from the top of the strip
float vImpact[NUM_BALLS];                 // As time goes on the impact velocity will change, so make an array to store those values
float tCycle[NUM_BALLS];                  // The time since the last time the ball struck the ground
int pos[NUM_BALLS];                       // The integer position of the dot on the strip (LED index)
long tLast[NUM_BALLS];                    // The clock time of the last ground strike
float COR[NUM_BALLS];                     // Coefficient of Restitution (bounce damping)

//VU 
uint8_t volCountLeft = 0;           // Frame counter for storing past volume data
int volLeft[SAMPLES];               // Collection of prior volume samples
int lvlLeft = 0;                    // Current "dampened" audio level
int minLvlAvgLeft = 0;              // For dynamic adjustment of graph low & high
int maxLvlAvgLeft = 512;
bool is_centered = false;          //For VU1 effects

// Effects library
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
// VU
#include "common.h"
#include "vu1.h"
#include "vu2.h"
#include "vu3.h"
#include "vu4.h"
#include "vu5.h"
#include "vu7.h"
#include "vu6.h"

// ----------------------------------------------------------------------------
// Definition of the LED component
// ----------------------------------------------------------------------------

struct Led
{
    // state variables
    uint8_t pin;
    bool on;

    // methods for update state of onboard led
    void update()
    {
        digitalWrite(pin, on ? HIGH : LOW);
    }
};

struct StripLed
{
    // state variables
    int R;
    int G;
    int B;
    int brightness;
    int effectId;
    bool powerState;

    // methods for different effects on stripled
    // SET ALL LEDS TO ONE COLOR
    void simpleColor(int ar, int ag, int ab, int brightness)
    {
        for (int i = 0; i < N_PIXELS; i++)
        {
            leds[i] = CRGB(ar, ag, ab);
        }
        FastLED.setBrightness(brightness);
        FastLED.show();
    }

    void runFire()
    {
        Fire fire = Fire();
        fire.runPattern();
    }

    void runMovingDot()
    {
        MovingDot movingDot = MovingDot();
        movingDot.runPattern();
    }

    void runRainbowBeat()
    {
        RainbowBeat rainbowBeat = RainbowBeat();
        rainbowBeat.runPattern();
    }

    void runRedWhiteBlue()
    {
        RedWhiteBlue redWhiteBlue = RedWhiteBlue();
        redWhiteBlue.runPattern();
    }

    void runRipple()
    {
        Ripple ripple = Ripple();
        ripple.runPattern();
    }

    void runTwinkle()
    {
        Twinkle twinkle = Twinkle();
        twinkle.runPattern();
    }

    void runBalls()
    {
        Balls balls = Balls();
        balls.runPattern();
    }

    void runJuggle()
    {
        Juggle juggle = Juggle();
        juggle.runPattern();
    }

    void runSinelon()
    {
        Sinelon sinelon = Sinelon();
        sinelon.runPattern();
    }

    void runComet()
    {
        Comet comet = Comet();
        comet.runPattern();
    }

    void runRainbowVU()
    {
        RainbowVU VU1 = RainbowVU();
        VU1.runPattern(is_centered, 0);
    }

    void runOldVU()
    {
        OldskoolVU VU2 = OldskoolVU();
        VU2.runPattern(is_centered, 0);
    }

    void runRainbowHueVU()
    {
        RainbowHueVU VU3 = RainbowHueVU();
        VU3.runPattern(is_centered, 0);
    }

    void runRippleVU()
    {
        RippleVU VU4 = RippleVU();
        VU4.runPattern(false);
    }

    void runThreebarsVU()
    {
        ThreebarsVU VU5 = ThreebarsVU();
        VU5.runPattern();
    }


    void update()
    {
        switch (effectId)
        {
        case 0:
            simpleColor(r, g, b, brightness);
            break;
        case 1:
            runFire();
            break;
        case 2:
            runMovingDot();
            break;
        case 3:
            runRainbowBeat();
            break;
        case 4:
            runRedWhiteBlue();
            break;
        case 5:
            runRipple();
            break;
        case 6:
            runTwinkle();
            break;
        case 7:
            runBalls();
            break;
        case 8:
            runJuggle();
            break;
        case 9:
            runSinelon();
            break;
        case 10:
            runComet();
            break;
        case 11:
            runRainbowVU();
            break;
        case 12:
            runOldVU();
            break;
        case 13:
            runRainbowHueVU();
            break;
        case 14:
            runRippleVU();
            break; 
        case 15:
            runThreebarsVU();
            break;            
        default:
            break;
        }
    }

    void clear()
    {
        FastLED.clear(); // clear all pixel data
        FastLED.show();
    }
};

// ----------------------------------------------------------------------------
// Definition of global variables
// ----------------------------------------------------------------------------

StripLed stripLed = {r, g, b, brightness, effectId, false};
Led onboard_led = {LED_BUILTIN, false};

// ----------------------------------------------------------------------------
// SPIFFS initialization
// ----------------------------------------------------------------------------

void initSPIFFS()
{
    if (!SPIFFS.begin())
    {
        Serial.println("Cannot mount SPIFFS volume...");
        while (1)
        {
            onboard_led.on = millis() % 200 < 50;
            onboard_led.update();
        }
    }
}

// ----------------------------------------------------------------------------
// Connecting to the WiFi network
// ----------------------------------------------------------------------------

void initWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.printf("Trying to connect [%s] ", WiFi.macAddress().c_str());
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }
    Serial.printf(" %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Listo!\nAbre http://%s.local en navegador\n", WEB_NAME);
    Serial.print("o en la IP: ");
    Serial.println(WiFi.localIP());

    if (!MDNS.begin(WEB_NAME))
    {
        Serial.println("Error configurando mDNS!");
        while (1)
        {
            delay(1000);
        }
    }
    Serial.println("mDNS configurado");
}

// ----------------------------------------------------------------------------
// Web server initialization
// ----------------------------------------------------------------------------

String processor(const String &var)
{
    if (var == "FIRE_STATE")
    {
        return String("off");
    }
    else if (var == "MOVINGDOT_STATE")
    {
        return String("off");
    }
    else if (var == "RAINBOWBEAT_STATE")
    {
        return String("off");
    }
    else if (var == "RWB_STATE")
    {
        return String("off");
    }
    else if (var == "RIPPLE_STATE")
    {
        return String("off");
    }
    else if (var == "TWINKLE_STATE")
    {
        return String("off");
    }
    else if (var == "BALLS_STATE")
    {
        return String("off");
    }
    else if (var == "JUGGLE_STATE")
    {
        return String("off");
    }
    else if (var == "SINELON_STATE")
    {
        return String("off");
    }
    else if (var == "COMET_STATE")
    {
        return String("off");
    }
    else if (var == "BRIGHTNESS")
    {
        return String(brightness);
    }
    else if (var == "COLOR")
    {
        const uint8_t array_size = JSON_ARRAY_SIZE(4);
        StaticJsonDocument<array_size> doc;
        doc["color"]["r"] = stripLed.R;
        doc["color"]["g"] = stripLed.G;
        doc["color"]["b"] = stripLed.B;
        char buffer_size[40];
        serializeJson(doc, buffer_size);
        return String(buffer_size);
    }
    else if (var == "NEOPIXEL")
    {
        return String(var == "NEOPIXEL" && stripLed.powerState ? "on" : "off");
    }
    else if (var == "BLUETOOTH")
    {
        return String(var == "BLUETOOTH" && bt_powerState ? "on" : "off");
    }
    else if (var == "VU1")
    {
        return String("off");
    }
    else if (var == "VU2")
    {
        return String("off");
    }
    else if (var == "VU3")
    {
        return String("off");
    }
    else if (var == "VU4")
    {
        return String("off");
    }
    else if (var == "VU5")
    {
        return String("off");
    }

    return String();
}

void onRootRequest(AsyncWebServerRequest *request)
{
    request->send(SPIFFS, "/index.html", "text/html", false, processor);
}

// Initialize webserver URLs
void initWebServer()
{
    server.on("/", onRootRequest);
    server.on("/wifi-info", HTTP_GET, [](AsyncWebServerRequest *request)
              {
      AsyncResponseStream *response = request->beginResponseStream("application/json");
      DynamicJsonDocument json(1024);
      json["status"] = "ok";
      json["ssid"] = WiFi.SSID();
      json["ip"] = WiFi.localIP().toString();
      json["rssi"] = WiFi.RSSI();
      serializeJson(json, *response);
      request->send(response); });

    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
    server.onNotFound([](AsyncWebServerRequest *request)
                      { request->send(400, "text/plain", "Not found"); });
    server.begin();
    Serial.println("HTTP server started");
    MDNS.addService("http", "tcp", 80);
}

// ----------------------------------------------------------------------------
// WebSocket initialization
// ----------------------------------------------------------------------------
String bars()
{
    int signal = WiFi.RSSI();
    switch (signal)
    {
    case -63 ... - 1:
        return "waveStrength-4";
        break;
    case -73 ... - 64:
        return "waveStrength-3";
        break;
    case -83 ... - 74:
        return "waveStrength-2";
        break;
    case -93 ... - 84:
        return "waveStrength-1";
        break;
    default:
        return "no-signal";
    }
}

void notifyClients()
{
    //Serial.println(bt_powerState);
    const int size = JSON_OBJECT_SIZE(20); // Remember change the number of member object
    StaticJsonDocument<size> json;
    json["bars"] = bars();
    json["neostatus"] = stripLed.powerState ? "on" : "off";
    json["btstatus"] = bt_powerState ? "on" : "off";
    json["neobrightness"] = stripLed.brightness;
    json["fireStatus"] = stripLed.effectId == 1 && stripLed.powerState ? "on" : "off";
    json["movingdotStatus"] = stripLed.effectId == 2 && stripLed.powerState ? "on" : "off";
    json["rainbowbeatStatus"] = stripLed.effectId == 3 && stripLed.powerState ? "on" : "off";
    json["rwbStatus"] = stripLed.effectId == 4 && stripLed.powerState ? "on" : "off";
    json["rippleStatus"] = stripLed.effectId == 5 && stripLed.powerState ? "on" : "off";
    json["twinkleStatus"] = stripLed.effectId == 6 && stripLed.powerState ? "on" : "off";
    json["ballsStatus"] = stripLed.effectId == 7 && stripLed.powerState ? "on" : "off";
    json["juggleStatus"] = stripLed.effectId == 8 && stripLed.powerState ? "on" : "off";
    json["sinelonStatus"] = stripLed.effectId == 9 && stripLed.powerState ? "on" : "off";
    json["cometStatus"] = stripLed.effectId == 10 && stripLed.powerState ? "on" : "off";
    //VU
    json["rainbowVUStatus"] = stripLed.effectId == 11 && bt_powerState ? "on" : "off";
    json["oldVUStatus"] = stripLed.effectId == 12 && bt_powerState ? "on" : "off";
    json["rainbowHueVUStatus"] = stripLed.effectId == 13 && bt_powerState ? "on" : "off";
    json["rippleVUStatus"] = stripLed.effectId == 14 && bt_powerState ? "on" : "off";
    json["threebarsVUStatus"] = stripLed.effectId == 15 && bt_powerState ? "on" : "off";
    char buffer[420];                         // the sum of all character {"stripledStatus":"off"} has 24 character and rainbow+theater= 46, total 70
    size_t len = serializeJson(json, buffer); // serialize the json+array and send the result to buffer
    ws.textAll(buffer, len);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
    {
        const uint8_t size = JSON_OBJECT_SIZE(4) + JSON_ARRAY_SIZE(3);
        StaticJsonDocument<size> json;
        DeserializationError err = deserializeJson(json, data);
        if (err)
        {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(err.c_str());
            return;
        }

        const char *action = json["action"];
        const int effectId = json["effectId"];
        stripLed.effectId = effectId;

        if (strcmp(action, "toggle") == 0)
        {
            stripLed.powerState = !stripLed.powerState;
            if (stripLed.powerState)
            {
                stripLed.update();
            }
            else
            {
                stripLed.clear();
            }
        }
        else if (strcmp(action, "animation") == 0)
        {
            if (stripLed.powerState)
            {
                stripLed.update();
            }
        }
        else if (strcmp(action, "slider") == 0)
        {
            const int brightness = json["brightness"].as<int>();
            Serial.println("Brillo " + String(brightness));
            stripLed.brightness = brightness;
        }
        else if (strcmp(action, "picker") == 0)
        {
            JsonArray color = json["color"];
            r = color[0].as<int>();
            g = color[1].as<int>();
            b = color[2].as<int>();
            Serial.print("RGB:  ");
            Serial.print(r);
            Serial.print(", ");
            Serial.print(g);
            Serial.print(", ");
            Serial.println(b);
            stripLed.R = r;
            stripLed.G = g;
            stripLed.B = b;
        }
        else if (strcmp(action, "music") == 0)
        {
            bt_powerState = !bt_powerState;
            if (bt_powerState)
            {
                digitalWrite(SWITCH_PIN,HIGH);
                bt_powerState = true;
                Serial.println("Encendido del modulo BT");
            }
            else
            {
                digitalWrite(SWITCH_PIN,LOW);
                bt_powerState = false;
                Serial.println("Apagado del modulo BT");
            }
        }

        else if (strcmp(action, "vu") == 0)
        {
            stripLed.powerState = !stripLed.powerState;
            if (bt_powerState && !stripLed.powerState)
            {
                stripLed.update();
            } else
                stripLed.powerState = false;
                stripLed.clear();
        }
        notifyClients();
    }
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
    case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
    case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
    case WS_EVT_PONG:
        Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
    case WS_EVT_ERROR:
        Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
        break;
    }
}

void initWebSocket()
{
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
    Serial.println("WebSocket server started");
}

// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

void setup()
{
    pinMode(onboard_led.pin, OUTPUT);
    pinMode(STRIP_PIN, OUTPUT);
    pinMode(SWITCH_PIN, OUTPUT);

    Serial.begin(115200);
    delay(500);
    
    FastLED.addLeds<LED_TYPE, STRIP_PIN, COLOR_ORDER>(leds, N_PIXELS).setCorrection(TypicalLEDStrip);
    FastLED.setMaxPowerInVoltsAndMilliamps(VOLTS, MAX_MILLIAMPS);
    FastLED.setBrightness(brightness);
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

    initSPIFFS();
    initWiFi();
    initWebSocket();
    initWebServer();
}

// ----------------------------------------------------------------------------
// Main control loop
// ----------------------------------------------------------------------------

void loop()
{
    ws.cleanupClients();

#if defined(ESP8266)
    MDNS.update();
#endif

    if (stripLed.powerState && !bt_powerState)
    {
        brightness = stripLed.brightness;
        stripLed.update();
        delay(6);
    } else if (!stripLed.powerState && stripLed.effectId >= 11)
    {
        stripLed.update();
    } else
        stripLed.powerState = false;
        stripLed.clear();
    

    onboard_led.on = millis() % 1000 < 50;
    onboard_led.update();

    currentMillis = millis();
    if (currentMillis - startMillis >= refresh) // Check the period has elapsed
    {
        notifyClients();
        startMillis = currentMillis;
    }
}