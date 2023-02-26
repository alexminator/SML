/**
 * ----------------------------------------------------------------------------
 * SMART MUSIC LAMP
 * ----------------------------------------------------------------------------
 * © 2023 Alexminator
 * ----------------------------------------------------------------------------
 */

#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

// ----------------------------------------------------------------------------
// Definition of macros
// ----------------------------------------------------------------------------
// Strip LED
#define LED_PIN 4
#define STRIP_NUMBER_LEDS 24
// WEB
#define HTTP_PORT 80
// Effects ID
#define SIMPLE_COLOR 0
#define RAINBOW 1

// ----------------------------------------------------------------------------
// Definition of global constants
// ----------------------------------------------------------------------------

// WiFi credentials
const char *WIFI_SSID = "MyWiFi";
const char *WIFI_PASS = "asd369/*";
// Strip LED
int ledBrightness = 50;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(STRIP_NUMBER_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
long hueNow = 0;
uint32_t colorNow = strip.Color(255, 0, 0); // Initial color

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
    int effectId;
    // methods for different effects on stripled
    void simpleColor(uint32_t ColorNow)
    {
        for (int i = 0; i < strip.numPixels(); i++)
        {
            strip.setPixelColor(i, ColorNow);
            delay(20);
        }
        strip.show();
        Serial.println("Color rojo ON");
    }

    void rainbowcolor()
    {
        strip.rainbow(hueNow);
        hueNow += 256;
        if (hueNow > 65536)
        {
            hueNow = 0;
        }
        strip.show();
    }
    void update()
    {
        switch (effectId)
        {
        case 0:
            simpleColor(colorNow);
            break;
        case 1:
            rainbowcolor();
            break;
        default:
            break;
        }
    }
};

struct Strip
{
    // state variables
    StripLed stripLed;
    bool powerState;

    // methods for main poweroff stripled
    void clear()
    {
        strip.clear();
        strip.show();
    }
};

// ----------------------------------------------------------------------------
// Definition of global variables
// ----------------------------------------------------------------------------

StripLed simple = {SIMPLE_COLOR};
StripLed rainbow = {RAINBOW};

Led onboard_led = {LED_BUILTIN, false};
Strip stripLed = {simple, false};

AsyncWebServer server(HTTP_PORT);
AsyncWebSocket ws("/ws");

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
}

// ----------------------------------------------------------------------------
// Web server initialization
// ----------------------------------------------------------------------------

String processor(const String &var)
{
    if (var == "RAINBOW_STATE")
    {
        return String("off");
    }
    else if (var == "STATE")
    {
        return String(var == "STATE" && stripLed.powerState ? "on" : "off");
    }
    return String();
}

void onRootRequest(AsyncWebServerRequest *request)
{
    request->send(SPIFFS, "/index.html", "text/html", false, processor);
}

void initWebServer()
{
    server.on("/", onRootRequest);
    server.serveStatic("/", SPIFFS, "/");
    server.begin();
}

// ----------------------------------------------------------------------------
// WebSocket initialization
// ----------------------------------------------------------------------------

void notifyClients()
{
    const uint8_t size = JSON_OBJECT_SIZE(2);
    StaticJsonDocument<size> json;
    json["status"] = stripLed.powerState ? "on" : "off";
    Serial.println("RanibowStatus: " + String(stripLed.stripLed.effectId == 1 && stripLed.powerState));
    json["rainbowStatus"] = stripLed.stripLed.effectId == 1 && stripLed.powerState ? "on" : "off";

    char buffer[50];
    size_t len = serializeJson(json, buffer);
    ws.textAll(buffer, len);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
    {

        const uint8_t size = JSON_OBJECT_SIZE(3);
        StaticJsonDocument<size> json;
        DeserializationError err = deserializeJson(json, data);
        if (err)
        {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(err.c_str());
            return;
        }

        const char *action = json["action"];
        if (strcmp(action, "toggle") == 0)
        {
            stripLed.powerState = !stripLed.powerState;
            if (stripLed.powerState)
            {
                stripLed.stripLed.update();
            }
            else
            {
                stripLed.clear();
            }
        }
        else if (strcmp(action, "animation") == 0)
        {
            const int effectId = json["effectId"];
            if (stripLed.powerState)
            {
                stripLed.stripLed.effectId = effectId;
                stripLed.stripLed.update();
            }
        }

        notifyClients();
    }
}

void onEvent(AsyncWebSocket *server,
             AsyncWebSocketClient *client,
             AwsEventType type,
             void *arg,
             uint8_t *data,
             size_t len)
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
    case WS_EVT_ERROR:
        break;
    }
}

void initWebSocket()
{
    ws.onEvent(onEvent);
    server.addHandler(&ws);
}

// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

void setup()
{
    pinMode(onboard_led.pin, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    
    Serial.begin(115200);
    delay(500);

    initSPIFFS();
    initWiFi();
    initWebSocket();
    initWebServer();

    strip.begin();
    strip.setBrightness(ledBrightness);
    strip.clear();
    strip.show();
}

// ----------------------------------------------------------------------------
// Main control loop
// ----------------------------------------------------------------------------

void loop()
{
    ws.cleanupClients();

    if (stripLed.powerState && stripLed.stripLed.effectId == 1)
    {
        stripLed.stripLed.update();
        delay(5);
    }

    onboard_led.on = millis() % 1000 < 50;
    onboard_led.update();
}