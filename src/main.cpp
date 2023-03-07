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
String strength;
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
        }
        strip.show();
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

// Refresh web signal info
unsigned long startMillis;
unsigned long currentMillis;
const unsigned long refresh = 3000;

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
    else if (var == "SSID")
    {
        return String(WiFi.SSID());
    }
    else if (var == "RSSI")
    {
        return String(WiFi.RSSI());
    }
    /*
    else if (var == "BARS")
    {
        int signal = WiFi.RSSI();
        switch (signal)
        {
        case -63 ... - 1:
            return String("four-bars");
        case -73 ... - 64:
            return String("three-bars");
        case -83 ... - 74:
            return String("two-bars");
        case -93 ... - 84:
            return String("one-bar");
        default:
            return String("no-signal");
        }
    }
    */
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

    server.serveStatic("/", SPIFFS, "/");
    server.begin();
}

// ----------------------------------------------------------------------------
// WebSocket initialization
// ----------------------------------------------------------------------------
String bars() {
    int signal = WiFi.RSSI();
    switch (signal)
        {
        case -63 ... - 1:
            return "signal-bars mt1 sizing-box four-bars";
            break;
        case -73 ... - 64:
            return "signal-bars mt1 sizing-box three-bars";
            break;
        case -83 ... - 74:
            return "signal-bars mt1 sizing-box two-bars";
            break;
        case -93 ... - 84:
            return "signal-bars mt1 sizing-box one-bar";
            break;
        default:
            return "no-signal";
        }
}

void notifyClients()
{
    const uint8_t size = JSON_OBJECT_SIZE(7); // Remember change the number of member object
    StaticJsonDocument<size> json;
    json["signalStrength"] = WiFi.RSSI();
    json["bars"] = bars();
    json["status"] = stripLed.powerState ? "on" : "off";
    json["rainbowStatus"] = stripLed.stripLed.effectId == 1 && stripLed.powerState ? "on" : "off";
    char buffer[120]; // I'ts 80 because {"stripledStatus":"off"} has 24 character and rainbow+theater= 46, total 70
    size_t len = serializeJson(json, buffer);
    ws.textAll(buffer, len);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
    {

        const uint8_t size = JSON_OBJECT_SIZE(4);
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
    
    if (stripLed.powerState)
    {
        stripLed.stripLed.update();
        delay(6);
    }

    onboard_led.on = millis() % 1000 < 50;
    onboard_led.update();

    currentMillis = millis();
    if (currentMillis - startMillis >= refresh) // Check the period has elapsed
    {
        notifyClients();
        startMillis = currentMillis;
    }
}