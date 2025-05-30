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
#define FADE_RATE 2 // How long should the trails be. Very low value = longer trails.
uint8_t volCount = 0; // Frame counter for storing past volume data
int vol[SAMPLES];     // Collection of prior volume samples
int lvl = 0;          // Current "dampened" audio level

CRGBPalette16 currentPalette; // Define the current palette
CRGBPalette16 targetPalette;  // Define the target palette

int brightness = 130;
int effectId = 0;
uint8_t myhue = 0;           // hue 0. red color
uint8_t r = 255;
uint8_t g = 255;
uint8_t b = 255;
CRGB leds[N_PIXELS];        //  for FASTLED

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
int lvlCharge;
Battery18650Stats battery(ADC_PIN, CONV_FACTOR, READS, MAXV, MINV);

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
#include "Battery.h"
// VU
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

    void runBattery()
    {
        Charge batt = Charge();
        batt.runPattern(lvlCharge);
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
        case 18:
            runBattery();
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
    sensors_event_t event;
    int retryCount = 0;
    const int maxRetries = 3; // Maximum number of retries
    const unsigned long retryDelay = 2000; // Delay between retries (2 seconds)
    while (retryCount < maxRetries)
    {
        dht.temperature().getEvent(&event);
        if (!isnan(event.temperature))
        {
            temp = event.temperature;
            #ifdef DHT
            debuglnD("Temperature: " + String(temp) + "°C");
            #endif
            break; // Exit loop on success
        }
        else
        {
            debuglnD("Error reading temperature! Retrying...");
            retryCount++;
            vTaskDelay(pdMS_TO_TICKS(retryDelay)); // Wait before retrying
        }
    }
    retryCount = 0; // Reset retry count for humidity reading
    while (retryCount < maxRetries)
    {
        dht.humidity().getEvent(&event);
        if (!isnan(event.relative_humidity))
        {
            hum = event.relative_humidity;
            #ifdef DHT
            debuglnD("Humidity: " + String(hum) + "%");
            #endif
            break; // Exit loop on success
        }
        else
        {
            debuglnD("Error reading humidity! Retrying...");
            retryCount++;
            vTaskDelay(pdMS_TO_TICKS(retryDelay)); // Wait before retrying
        }
    }
}

// ----------------------------------------------------------------------------
// SPIFFS initialization
// ----------------------------------------------------------------------------

void initLittleFS()
{
    if (!LittleFS.begin())
    {
        debuglnD("Cannot mount LittleFS volume...");
        while (1)
        {
            onboard_led.on = millis() % 200 < 50; // LED flashes, lighting for 50 ms and turning off for 150 ms in a 200 ms cycle. Indicates error when mounting volume
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
        vTaskDelay(pdMS_TO_TICKS(500));
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
            vTaskDelay(pdMS_TO_TICKS(1000));
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
    TEMPNEO,
    BATTNEO
} status;

String processor(const String &var)
{
    switch (status)
    {
    case COLOR:
    {
        StaticJsonDocument<JSON_ARRAY_SIZE(4)> doc;
        doc["color"]["r"] = stripLed.R;
        doc["color"]["g"] = stripLed.G;
        doc["color"]["b"] = stripLed.B;
        char buffer[40];
        serializeJson(doc, buffer);
        return String(buffer);
    }
    
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
    case BATTNEO:
    case LAMP:
        return "off"; 
    case BRIGHTNESS:
        return String(brightness);
    case STRIPLED:
        return stripLed.powerState ? "on" : "off";
    case BLUETOOTH:
        return bt_powerState ? "on" : "off";
    default:
        return String();
    }
}

void onRootRequest(AsyncWebServerRequest *request)
{
    request->send(LittleFS, "/index.html", "text/html", false, processor);
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

    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
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
    StaticJsonDocument<JSON_OBJECT_SIZE(36)> json;      // !Remember change the number of member object. See https://arduinojson.org/v5/assistant/
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

    // Añade el color actual
    JsonObject color = json.createNestedObject("color");
    color["r"] = stripLed.R;
    color["g"] = stripLed.G;
    color["b"] = stripLed.B;

    // Efectos y VU
    const char *effectNames[] = {
        "fireStatus", "movingdotStatus", "rainbowbeatStatus", "rwbStatus", "rippleStatus",
        "twinkleStatus", "ballsStatus", "juggleStatus", "sinelonStatus", "cometStatus",
        "rainbowVUStatus", "oldVUStatus", "rainbowHueVUStatus", "rippleVUStatus",
        "threebarsVUStatus", "oceanVUStatus", "tempNEOStatus", "battNEOStatus"};
    for (uint8_t i = 0; i < 18; ++i)
        json[effectNames[i]] = (stripLed.effectId == i + 1 && stripLed.powerState) ? "on" : "off";

    char buffer[650];                                  // the sum of all character of json send {"stripledStatus":"off"}
    size_t len = serializeJson(json, buffer);         // serialize the json+array and send the result to buffer     
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
            JsonObject color = json["color"];
            stripLed.R = r = color["r"].as<int>();
            stripLed.G = g = color["g"].as<int>();
            stripLed.B = b = color["b"].as<int>();
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
            vTaskDelay(pdMS_TO_TICKS(long_delay));
            digitalWrite(VOLUMENUP_PIN, LOW); // Deactivate Mosfet, release button
        }
        else if (strcmp(action, "voldown") == 0)
        {
            digitalWrite(VOLUMENDOWN_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(long_delay));
            digitalWrite(VOLUMENDOWN_PIN, LOW);
        }
        else if (strcmp(action, "skipL") == 0)
        {
            digitalWrite(VOLUMENDOWN_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(short_delay));
            digitalWrite(VOLUMENDOWN_PIN, LOW);
        }
        else if (strcmp(action, "skipR") == 0)
        {
            digitalWrite(VOLUMENUP_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(short_delay));
            digitalWrite(VOLUMENUP_PIN, LOW);
        }
        else if (strcmp(action, "play-pause") == 0)
        {
            digitalWrite(PLAY_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(short_delay));
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
// RTOS Tasks
// ----------------------------------------------------------------------------

TaskHandle_t TaskWebSocketHandle;
TaskHandle_t TaskBatteryMonitorHandle;
TaskHandle_t TaskLEDControlHandle;
TaskHandle_t TaskWiFiMonitorHandle;
TaskHandle_t TaskSensorHandle;
TaskHandle_t TaskOnboardLEDHandle;

void TaskWebSocket(void *pvParameters)
{
    while (true)
    {
        ws.cleanupClients();
        notifyClients();
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

void TaskBatteryMonitor(void *pvParameters)
{
    while (true)
    {
        batt.battMonitor();
        lvlCharge = batt.battLvl;
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

void TaskLEDControl(void *pvParameters)
{
    while (true)
    {
        if (stripLed.powerState)
        {
            brightness = stripLed.brightness;
            stripLed.update();
        }
        else
        {
            stripLed.clear();
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void TaskWiFiMonitor(void *pvParameters)
{
    while (true)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            ESP.restart();
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void TaskSensor(void *pvParameters)
{
    while (true)
    {
        readSensor();
        //UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(TaskSensorHandle); // give me a mark of 716
        //Serial.printf("TaskSensor stack high water mark: %u\n", highWaterMark);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void TaskOnboardLED(void *pvParameters)
{
    while (true)
    {
        onboard_led.on = millis() % 1000 < 500; // Blink every 1000 ms (on for 500 ms)
        onboard_led.update();
        vTaskDelay(pdMS_TO_TICKS(100)); // Update every 100 ms
    }
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
    debuglnD("Temperature Sensor\nSensor Type: " + String(sensor.name) + "\nDriver Ver: " + String(sensor.version) + "\nUnique ID: " + String(sensor.sensor_id) + "\nMax Value: " + String(sensor.max_value) + "°C\nMin Value: " + String(sensor.min_value) + "°C\nResolution: " + String(sensor.resolution) + "°C\n----------------------------------------------------");
    // Print humidity sensor details.
    dht.humidity().getSensor(&sensor);
    debuglnD("Humidity Sensor\nSensor Type: " + String(sensor.name) + "\nDriver Ver: " + String(sensor.version) + "\nUnique ID: " + String(sensor.sensor_id) + "\nMax Value: " + String(sensor.max_value) + "%\nMin Value: " + String(sensor.min_value) + "%\nResolution: " + String(sensor.resolution) + "%\n----------------------------------------------------");
    
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

    initLittleFS();
    initWiFi();
    initWebSocket();
    initWebServer();

    xTaskCreatePinnedToCore(TaskWebSocket, "WebSocketTask", 4096, NULL, 1, &TaskWebSocketHandle, 0);
    xTaskCreatePinnedToCore(TaskBatteryMonitor, "BatteryMonitorTask", 2048, NULL, 1, &TaskBatteryMonitorHandle, 1);
    xTaskCreatePinnedToCore(TaskLEDControl, "LEDControlTask", 2048, NULL, 1, &TaskLEDControlHandle, 0);
    xTaskCreatePinnedToCore(TaskWiFiMonitor, "WiFiMonitorTask", 2048, NULL, 1, &TaskWiFiMonitorHandle, 1);
    xTaskCreatePinnedToCore(TaskSensor, "SensorTask", 2048, NULL, 1, &TaskSensorHandle, 0);
    xTaskCreatePinnedToCore(TaskOnboardLED, "LEDOnboardTask", 2048, NULL, 1, &TaskOnboardLEDHandle, 1);
}

// ----------------------------------------------------------------------------
// Main control loop
// ----------------------------------------------------------------------------

void loop()
{

}
