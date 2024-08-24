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
#include <FastLED.h>
#include <Battery18650Stats.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include "data.h"

// Declare the debugging level then include the header file
#define DEBUGLEVEL DEBUGLEVEL_DEBUGGING
// #define DEBUGLEVEL DEBUGLEVEL_NONE
#include "debug.h"
// Declare what message you want to display on the console.
// User picks console message from this list
// This selection will not be effective if DEBUGLEVEL is DEBUGLEVEL_NONE
// #define DHT
// #define BATTERY
#define INDICATOR

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
// VU
#define AUDIO_IN_PIN 36    // Aux in signal
#define DC_OFFSET 0        // DC offset in aux signal [0]
#define NOISE 30           // Noise/hum/interference in aux signal [10]
#define SAMPLES 60         // Length of buffer for dynamic level adjustment [60]
#define TOP (N_PIXELS + 2) // Allow dot to go slightly off scale [(N_PIXELS + 2)]
#define PEAK_FALL 20       // Rate of peak falling dot [20]
#define N_PIXELS_HALF (N_PIXELS / 2)
#define BIAS 1850 // ADC value for HALF of 3.22V VCC. Hint: Take the analog reading without signal
// Effects
#define GRAVITY -1  // Downward (negative) acceleration of gravity in m/s^2
#define h0 1        // Starting height, in meters, of the ball (strip length)
#define NUM_BALLS 3 // Number of bouncing balls you want (recommend < 7, but 20 is fun in its own way)

uint8_t volCount = 0; // Frame counter for storing past volume data
int vol[SAMPLES];     // Collection of prior volume samples
int lvl = 0;          // Current "dampened" audio level
int minLvlAvg = 0;    // For dynamic adjustment of graph low & high
int maxLvlAvg = 512;

CRGBPalette16 currentPalette; // Define the current palette
CRGBPalette16 targetPalette;  // Define the target palette

// WEB
#define HTTP_PORT 80
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

AsyncWebServer server(HTTP_PORT);
AsyncWebSocket ws("/ws");

// ----------------------------------------------------------------------------
// Definition of global constants
// ----------------------------------------------------------------------------

// DHT Sensor
#define DHTPIN 23 // Digital pin connected to the DHT sensor
// Uncomment the type of sensor in use:
// #define DHTTYPE   DHT11     // DHT 11
#define DHTTYPE DHT22 // DHT 22 (AM2302)
// #define DHTTYPE   DHT21     // DHT 21 (AM2301)
DHT_Unified dht(DHTPIN, DHTTYPE);
float temp;
float hum;

// Power Switch for Bluetooth Module
#define SWITCH_PIN 18 // Pin to command relay. BT on/off
bool bt_powerState = false;
// Emulate BT Button
#define VOLUMENUP_PIN 5                // Pin to command Mosfet to emulate a BT button Volumen UP and FF.
#define VOLUMENDOWN_PIN 19             // Pin to command Mosfet to emulate a BT button Volumen DOWN and REW.
#define PLAY_PIN 21                    // Pin to command Mosfet to emulate a BT button PLAY AND PAUSE.
const unsigned long long_delay = 1000; // More than 1s (Volumen + -)
const unsigned long short_delay = 200; // Short time (FF, RW, PLAY and PAUSE)
// Lamp Switch
#define LAMP_PIN 32 // Pin to command LAMP IN1 relay
bool lampState = false;
// Charge Status
#define CHARGE_PIN 34 // Pin to sense Charge mode, signal come from TP4056
// Sensor Battery
#define FULL_CHARGE_PIN 35 // Pin to sense full charge, signal come from TP4056
#define ADC_PIN 33         // Pin to monitor Batt
#define CONV_FACTOR 1.702
#define READS 30
#define MAXV 4.00
#define MINV 3.20
#define BATT_THRESHOLD 30 // Defines battery threshold in %
#define MAX_READS 10      // Defines the maximum number of readings when the battery reaches the threshold
#define FULL_READS 10     // Defines the maximum number of readings when the battery is full
double battVolts;
int battLvl;
int readCount = 0;
Battery18650Stats battery(ADC_PIN, CONV_FACTOR, READS, MAXV, MINV);

// Web signal info
unsigned long startMillis;
unsigned long currentMillis;
const unsigned long refresh = 3000UL; // 3 seg Unsigned long
String strength;

// Strip LED
int brightness = 130;
int effectId = 0;
CRGB leds[N_PIXELS];
uint8_t myhue = 0;           // hue 0. red color
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

// VU
uint8_t volCountLeft = 0; // Frame counter for storing past volume data
int volLeft[SAMPLES];     // Collection of prior volume samples
int lvlLeft = 0;          // Current "dampened" audio level
int minLvlAvgLeft = 0;    // For dynamic adjustment of graph low & high
int maxLvlAvgLeft = 512;
bool is_centered = false; // For VU1 effects

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
#include "Temp.h"
// VU
#include "common.h"
#include "vu1.h"
#include "vu2.h"
#include "vu3.h"
#include "vu4.h"
#include "vu5.h"
#include "vu6.h"

// ----------------------------------------------------------------------------
// Definition of Battery component
// ----------------------------------------------------------------------------

struct Battery
{
    // state variables
    double battVolts;
    int battLvl;
    bool fullBatt;
    bool chargeState;
    // methods for monitor battery
    void battMonitor() // Define a function to read the state of the battery and charge
    {
        // Read the state of the charge signal came from pin 7 of TP4056. Low level means charging mode
        int isCharging = digitalRead(CHARGE_PIN);
        chargeState = isCharging == LOW; // If "isCharging" is equal to "LOW", then "chargeState" is set to true, indicating that the battery is charging
        // Read the state of the charge signal came from pin 6 of TP4056. Low level means Full Battery charge
        int fullyCharge = digitalRead(FULL_CHARGE_PIN);
        fullBatt = fullyCharge == LOW; // If "fullyCharge" is equal to "LOW", then "fullBatt" is set to true, indicating that the battery is fully charged
        // Get the voltage and charge level of the battery using the Battery library
        battVolts = battery.getBatteryVolts();
        battLvl = battery.getBatteryChargeLevel(true);

// Print the obtained values
#ifdef BATTERY
        debuglnD(chargeState ? "Cargador conectado" : "Cargador desconectado"); // Print the charger status
        debuglnD("Estado del pin carga: " + String(fullyCharge));
        debuglnD(!fullBatt && !chargeState ? "Batería usándose" : (fullBatt ? "Batería completamente cargada" : "Batería cargándose")); // Print the battery status
        debuglnD("Lectura promedio del pin: " + String(battery.pinRead()) + ", Voltaje: " + String(battVolts) + ", Nivel de carga: " + String(battLvl));
#endif
    }
};

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
        VU4.runPattern(true);
    }

    void runThreebarsVU()
    {
        ThreebarsVU VU5 = ThreebarsVU();
        VU5.runPattern();
    }

    void runOceanVU()
    {
        OceanVU VU6 = OceanVU();
        VU6.runPattern();
    }

    void runTemperature()
    {
        Temperature temp = Temperature();
        temp.runPattern();
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
        case 16:
            runOceanVU();
            break;
        case 17:
            runTemperature();
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
// Definition of objects
// ----------------------------------------------------------------------------

StripLed stripLed = {r, g, b, brightness, effectId, false};
Led onboard_led = {LED_BUILTIN, false};
Battery batt = {battVolts, battLvl, false, false};

//-----------------------------------------------------------------------------
// DHT initialization
//-----------------------------------------------------------------------------
void readSensor()
{
    // Get temperature event and print its value.
    sensors_event_t event;
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature))
    {
        Serial.println(F("Error reading temperature!"));
    }
    else
    {
        temp = event.temperature;
#ifdef DHT
        debuglnD("Temperature: " + String(temp) + "°C");
#endif
    }
    // Get humidity event and print its value.
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity))
    {
        Serial.println(F("Error reading humidity!"));
    }
    else
    {
        hum = event.relative_humidity;
#ifdef DHT
        debuglnD("Humidity: " + String(hum) + "%");
#endif
    }
}

// ----------------------------------------------------------------------------
// SPIFFS initialization
// ----------------------------------------------------------------------------

void initSPIFFS()
{
    if (!SPIFFS.begin())
    {
        debuglnD("Cannot mount SPIFFS volume...");
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
        debuglnD("Error configurando mDNS!");
        while (1)
        {
            delay(1000);
        }
    }
    debuglnD("mDNS configurado");
}

// ----------------------------------------------------------------------------
// Web server initialization
// ----------------------------------------------------------------------------
enum Status
{
    COLOR,
    FIRE_STATE,
    MOVINGDOT_STATE,
    RAINBOWBEAT_STATE,
    RWB_STATE,
    RIPPLE_STATE,
    TWINKLE_STATE,
    BALLS_STATE,
    JUGGLE_STATE,
    SINELON_STATE,
    COMET_STATE,
    BRIGHTNESS,
    STRIPLED,
    BLUETOOTH,
    VU1,
    VU2,
    VU3,
    VU4,
    VU5,
    VU6,
    LAMP,
    TEMPNEO
} status;

String processor(const String &var)
{
    switch (status)
    {
    case COLOR:
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
    break;
    case FIRE_STATE:
    case MOVINGDOT_STATE:
    case RAINBOWBEAT_STATE:
    case RWB_STATE:
    case RIPPLE_STATE:
    case TWINKLE_STATE:
    case BALLS_STATE:
    case JUGGLE_STATE:
    case SINELON_STATE:
    case COMET_STATE:
    case VU1:
    case VU2:
    case VU3:
    case VU4:
    case VU5:
    case VU6:
    case TEMPNEO:
        return String("off");
        break;
    case LAMP:
        return String("off");
        break;
    case BRIGHTNESS:
        return String(brightness);
        break;
    case STRIPLED:
        return String(stripLed.powerState ? "on" : "off");
        break;
    case BLUETOOTH:
        return String(bt_powerState ? "on" : "off");
        break;
    default:
        return String();
    }
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
    ElegantOTA.begin(&server); // Start ElegantOTA
    server.begin();
    debuglnD("HTTP server started");
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
    const int size = JSON_OBJECT_SIZE(31); // Remember change the number of member object. See https://arduinojson.org/v5/assistant/
    StaticJsonDocument<size> json;
    json["bars"] = bars();
    json["battVoltage"] = String(batt.battVolts, 3);
    json["level"] = String(batt.battLvl);
    json["charging"] = batt.chargeState;
    json["fullbatt"] = batt.fullBatt;
    json["temperature"] = String(temp, 1);
    json["humidity"] = String(hum, 1);
    json["lampstatus"] = lampState ? "on" : "off";
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
    // VU
    json["rainbowVUStatus"] = stripLed.effectId == 11 && stripLed.powerState ? "on" : "off";
    json["oldVUStatus"] = stripLed.effectId == 12 && stripLed.powerState ? "on" : "off";
    json["rainbowHueVUStatus"] = stripLed.effectId == 13 && stripLed.powerState ? "on" : "off";
    json["rippleVUStatus"] = stripLed.effectId == 14 && stripLed.powerState ? "on" : "off";
    json["threebarsVUStatus"] = stripLed.effectId == 15 && stripLed.powerState ? "on" : "off";
    json["oceanVUStatus"] = stripLed.effectId == 16 && stripLed.powerState ? "on" : "off";
    // Indicators
    json["tempNEOStatus"] = stripLed.effectId == 17 && stripLed.powerState ? "on" : "off";
    char buffer[590];                         // the sum of all character of json send {"stripledStatus":"off"}
    size_t len = serializeJson(json, buffer); // serialize the json+array and send the result to buffer
    ws.textAll(buffer, len);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
    {
        const uint8_t size = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(4) + 66; // See https://arduinojson.org/v5/assistant/
        StaticJsonDocument<size> json;
        DeserializationError err = deserializeJson(json, data);
        if (err)
        {
            debuglnD(F("deserializeJson() failed with code "));
            debuglnD(err.c_str());
            return;
        }

        const char *action = json["action"];
        const int effectId = json["effectId"];
        stripLed.effectId = effectId;

        if (strcmp(action, "toggle") == 0)
        {
            stripLed.powerState = !stripLed.powerState;
            stripLed.powerState ? stripLed.update() : stripLed.clear();
        }
        else if (strcmp(action, "lamp") == 0)
        {
            lampState = !lampState;
            digitalWrite(LAMP_PIN, lampState ? LOW : HIGH);
            debuglnD(lampState ? "Lampara ON" : "Lampara OFF");
        }
        else if (strcmp(action, "animation") == 0 || strcmp(action, "vu") == 0 || strcmp(action, "indicator") == 0)
        {
            if (stripLed.powerState)
            {
                stripLed.update();
            }
        }
        else if (strcmp(action, "slider") == 0)
        {
            const int brightness = json["brightness"].as<int>();
            debuglnD("Brillo " + String(brightness));
            stripLed.brightness = brightness;
        }
        else if (strcmp(action, "picker") == 0)
        {
            JsonArray color = json["color"];
            stripLed.R = r = color[0].as<int>();
            stripLed.G = g = color[1].as<int>();
            stripLed.B = b = color[2].as<int>();
            debuglnD("RGB: " + String(r) + ", " + String(g) + ", " + String(b));
        }
        else if (strcmp(action, "music") == 0)
        {
            bt_powerState = !bt_powerState;
            digitalWrite(SWITCH_PIN, bt_powerState ? LOW : HIGH);
            bt_powerState ? Serial.println("Encendido del modulo BT") : Serial.println("Apagado del modulo BT");
        }
        else if (strcmp(action, "volup") == 0)
        {
            // Simulate a button press
            digitalWrite(VOLUMENUP_PIN, HIGH); // Activate Mosfet, push a button
            delay(long_delay);
            digitalWrite(VOLUMENUP_PIN, LOW); // Deactivate Mosfet, release button
        }
        else if (strcmp(action, "voldown") == 0)
        {
            digitalWrite(VOLUMENDOWN_PIN, HIGH);
            delay(long_delay);
            digitalWrite(VOLUMENDOWN_PIN, LOW);
        }
        else if (strcmp(action, "skipL") == 0)
        {
            digitalWrite(VOLUMENDOWN_PIN, HIGH);
            delay(short_delay);
            digitalWrite(VOLUMENDOWN_PIN, LOW);
        }
        else if (strcmp(action, "skipR") == 0)
        {
            digitalWrite(VOLUMENUP_PIN, HIGH);
            delay(short_delay);
            digitalWrite(VOLUMENUP_PIN, LOW);
        }
        else if (strcmp(action, "play-pause") == 0)
        {
            digitalWrite(PLAY_PIN, HIGH);
            delay(short_delay);
            digitalWrite(PLAY_PIN, LOW);
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
    debuglnD("WebSocket server started");
}

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

    // Initialize DHT22 device.
    dht.begin();
    // Print temperature sensor details.
    sensor_t sensor;
    dht.temperature().getSensor(&sensor);
    debuglnD("----------------------------------------------------\nTemperature Sensor\nSensor Type: " + String(sensor.name) + "\nDriver Ver: " + String(sensor.version) + "\nUnique ID: " + String(sensor.sensor_id) + "\nMax Value: " + String(sensor.max_value) + "°C\nMin Value: " + String(sensor.min_value) + "°C\nResolution: " + String(sensor.resolution) + "°C\n----------------------------------------------------");
    // Print humidity sensor details.
    dht.humidity().getSensor(&sensor);
    debuglnD("----------------------------------------------------\nHumidity Sensor\nSensor Type: " + String(sensor.name) + "\nDriver Ver: " + String(sensor.version) + "\nUnique ID: " + String(sensor.sensor_id) + "\nMax Value: " + String(sensor.max_value) + "%\nMin Value: " + String(sensor.min_value) + "%\nResolution: " + String(sensor.resolution) + "%\n----------------------------------------------------");
    
    // For FASTLED library
    FastLED.addLeds<LED_TYPE, STRIP_PIN, COLOR_ORDER>(leds, N_PIXELS).setCorrection(TypicalLEDStrip);
    FastLED.setMaxPowerInVoltsAndMilliamps(VOLTS, MAX_MILLIAMPS);
    FastLED.setBrightness(brightness);
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

    startMillis = millis(); // initial start time

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

    stripLed.powerState ? (brightness = stripLed.brightness, stripLed.update(), delay(6)) : stripLed.clear();

    onboard_led.on = millis() % 1000 < 50;
    onboard_led.update();

    currentMillis = millis();
    (currentMillis - startMillis >= refresh) ? (batt.battMonitor(), notifyClients(), readSensor(), startMillis = currentMillis) : 0;

    // Check WiFi connection status
    if (WiFi.status() != WL_CONNECTED)
    {
        // The ESP32 has been disconnected from the WiFi network
        ESP.restart(); // Restart the esp32
    }
}
