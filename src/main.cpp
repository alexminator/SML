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
#define STRIPLED_PIN 4
#define STRIP_NUMBER_LEDS 24
// Pattern types supported:
enum  pattern { COLOR, RAINBOW_CYCLE, THEATER_CHASE, COLOR_WIPE, SCANNER, FADE };
// Patern directions supported:
enum  direction { FORWARD, REVERSE };
// WEB
#define HTTP_PORT 80

// ----------------------------------------------------------------------------
// Definition of global constants
// ----------------------------------------------------------------------------

// WiFi credentials
const char *WIFI_SSID = "MyWiFi";
const char *WIFI_PASS = "asd369/*";
// Strip LED
int ledBrightness = 50;
long hueNow = 0;

// ----------------------------------------------------------------------------
// Definition of the LED component
// ----------------------------------------------------------------------------

class Led
{
    public:
     // state variables
    uint8_t pin;
    bool on;

    // methods for update state of onboard led
    void update()
    {
        digitalWrite(pin, on ? HIGH : LOW);
    }
};

// NeoPattern Class - derived from the Adafruit_NeoPixel class
class NeoPatterns : public Adafruit_NeoPixel
{
    public:

    // Member Variables:  
    pattern  ActivePattern;  // which pattern is running
    direction Direction;     // direction to run the pattern
    
    unsigned long Interval;   // milliseconds between updates
    unsigned long lastUpdate; // last update of position
    
    uint32_t Color1, Color2;  // What colors are in use
    uint16_t TotalSteps;  // total number of steps in the pattern
    uint16_t Index;  // current step within the pattern
    
    void (*OnComplete)();  // Callback on completion of pattern
    
    // Constructor - calls base-class constructor to initialize strip
    NeoPatterns(uint16_t pixels, uint8_t pin, uint8_t type, void (*callback)())
    :Adafruit_NeoPixel(pixels, pin, type)
    {
        OnComplete = callback;
    }
    
    // Update the pattern
    void Update()
    {
        if((millis() - lastUpdate) > Interval) // time to update
        {
            lastUpdate = millis();
            switch(ActivePattern) 
            {
                case COLOR:
                    ColorSet(Stick.Color(255, 0, 0)); // to do pasarle las variables R G B
                    break;
                case RAINBOW_CYCLE:
                    RainbowCycleUpdate();
                    break;
                case THEATER_CHASE:
                    TheaterChaseUpdate();
                    break;
                case COLOR_WIPE:
                    ColorWipeUpdate();
                    break;
                case SCANNER:
                    ScannerUpdate();
                    break;
                case FADE:
                    FadeUpdate();
                    break;
                default:
                    break;
            }
        }
    }
  
    // Increment the Index and reset at the end
    void Increment()
    {
        if (Direction == FORWARD)
        {
           Index++;
           if (Index >= TotalSteps)
            {
                Index = 0;
                if (OnComplete != NULL)
                {
                    OnComplete(); // call the comlpetion callback
                }
            }
        }
        else // Direction == REVERSE
        {
            --Index;
            if (Index <= 0)
            {
                Index = TotalSteps-1;
                if (OnComplete != NULL)
                {
                    OnComplete(); // call the comlpetion callback
                }
            }
        }
    }
    
    // Reverse pattern direction
    void Reverse()
    {
        if (Direction == FORWARD)
        {
            Direction = REVERSE;
            Index = TotalSteps-1;
        }
        else
        {
            Direction = FORWARD;
            Index = 0;
        }
    }
    
    // Initialize for a RainbowCycle
    void RainbowCycle(uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = RAINBOW_CYCLE;
        Interval = interval;
        TotalSteps = 255;
        Index = 0;
        Direction = dir;
    }
    
    // Update the Rainbow Cycle Pattern
    void RainbowCycleUpdate()
    {
        for(int i=0; i< numPixels(); i++)
        {
            setPixelColor(i, Wheel(((i * 256 / numPixels()) + Index) & 255));
        }
        show();
        Increment();
    }

    // Initialize for a Theater Chase
    void TheaterChase(uint32_t color1, uint32_t color2, uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = THEATER_CHASE;
        Interval = interval;
        TotalSteps = numPixels();
        Color1 = color1;
        Color2 = color2;
        Index = 0;
        Direction = dir;
   }
    
    // Update the Theater Chase Pattern
    void TheaterChaseUpdate()
    {
        for(int i=0; i< numPixels(); i++)
        {
            if ((i + Index) % 3 == 0)
            {
                setPixelColor(i, Color1);
            }
            else
            {
                setPixelColor(i, Color2);
            }
        }
        show();
        Increment();
    }

    // Initialize for a ColorWipe
    void ColorWipe(uint32_t color, uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = COLOR_WIPE;
        Interval = interval;
        TotalSteps = numPixels();
        Color1 = color;
        Index = 0;
        Direction = dir;
    }
    
    // Update the Color Wipe Pattern
    void ColorWipeUpdate()
    {
        setPixelColor(Index, Color1);
        show();
        Increment();
    }
    
    // Initialize for a SCANNNER
    void Scanner(uint32_t color1, uint8_t interval)
    {
        ActivePattern = SCANNER;
        Interval = interval;
        TotalSteps = (numPixels() - 1) * 2;
        Color1 = color1;
        Index = 0;
    }

    // Update the Scanner Pattern
    void ScannerUpdate()
    { 
        for (int i = 0; i < numPixels(); i++)
        {
            if (i == Index)  // Scan Pixel to the right
            {
                 setPixelColor(i, Color1);
            }
            else if (i == TotalSteps - Index) // Scan Pixel to the left
            {
                 setPixelColor(i, Color1);
            }
            else // Fading tail
            {
                 setPixelColor(i, DimColor(getPixelColor(i)));
            }
        }
        show();
        Increment();
    }
    
    // Initialize for a Fade
    void Fade(uint32_t color1, uint32_t color2, uint16_t steps, uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = FADE;
        Interval = interval;
        TotalSteps = steps;
        Color1 = color1;
        Color2 = color2;
        Index = 0;
        Direction = dir;
    }
    
    // Update the Fade Pattern
    void FadeUpdate()
    {
        // Calculate linear interpolation between Color1 and Color2
        // Optimise order of operations to minimize truncation error
        uint8_t red = ((Red(Color1) * (TotalSteps - Index)) + (Red(Color2) * Index)) / TotalSteps;
        uint8_t green = ((Green(Color1) * (TotalSteps - Index)) + (Green(Color2) * Index)) / TotalSteps;
        uint8_t blue = ((Blue(Color1) * (TotalSteps - Index)) + (Blue(Color2) * Index)) / TotalSteps;
        
        ColorSet(Color(red, green, blue));
        show();
        Increment();
    }
   
    // Calculate 50% dimmed version of a color (used by ScannerUpdate)
    uint32_t DimColor(uint32_t color)
    {
        // Shift R, G and B components one bit to the right
        uint32_t dimColor = Color(Red(color) >> 1, Green(color) >> 1, Blue(color) >> 1);
        return dimColor;
    }

    // Set all pixels to a color (synchronously)
    void ColorSet(uint32_t color)
    {
        for (int i = 0; i < numPixels(); i++)
        {
            setPixelColor(i, color);
        }
        show();
    }

    // Returns the Red component of a 32-bit color
    uint8_t Red(uint32_t color)
    {
        return (color >> 16) & 0xFF;
    }

    // Returns the Green component of a 32-bit color
    uint8_t Green(uint32_t color)
    {
        return (color >> 8) & 0xFF;
    }

    // Returns the Blue component of a 32-bit color
    uint8_t Blue(uint32_t color)
    {
        return color & 0xFF;
    }
    
    // Input a value 0 to 255 to get a color value.
    // The colours are a transition r - g - b - back to r.
    uint32_t Wheel(byte WheelPos)
    {
        WheelPos = 255 - WheelPos;
        if(WheelPos < 85)
        {
            return Color(255 - WheelPos * 3, 0, WheelPos * 3);
        }
        else if(WheelPos < 170)
        {
            WheelPos -= 85;
            return Color(0, WheelPos * 3, 255 - WheelPos * 3);
        }
        else
        {
            WheelPos -= 170;
            return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
        }
    }
};

class Strip
{
    public:
    // state variables
    bool powerState;
    
    // methods for main poweroff stripled
    void clear()
    {
        Stick.clear();
        Stick.show();
    }
};

void StickComplete()
{
    // Random color change for next scan
    //Stick.Color1 = Stick.Wheel(random(255));
    Stick.ColorSet(Stick.Color(255, 0, 0));  // All in RED color
}

// ----------------------------------------------------------------------------
// Definition of global variables
// ----------------------------------------------------------------------------
// Constructor
Led onboard_led = {LED_BUILTIN, false};
NeoPatterns Stick(STRIP_NUMBER_LEDS, STRIPLED_PIN, NEO_GRB + NEO_KHZ800, &StickComplete);
Strip stripled = {false};

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
    else if (var == "THEATER_STATE")
    {
        return String("off");
    }
    else if (var == "COLOR_STATE")
    {
        return String("off");
    }
    else if (var == "SCANNER_STATE")
    {
        return String("off");
    }
    else if (var == "FADE_STATE")
    {
        return String("off");
    }
    else if (var == "STRIPLED_STATE")
    {
        return String("off");
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
    const uint8_t size = JSON_OBJECT_SIZE(3);   //Remember change the number of member object
    StaticJsonDocument<size> json;
    json["stripledStatus"] = stripled.powerState ? "on" : "off";
    json["rainbowStatus"] = Stick.ActivePattern == RAINBOW_CYCLE && stripled.powerState ? "on" : "off";
    json["theaterStatus"] = Stick.ActivePattern == THEATER_CHASE && stripled.powerState ? "on" : "off";
    
    char buffer[25]; // I'ts 25 because {"stripledStatus":"off"} has 24 character. Change it if exists a bigger json string
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
            stripled.powerState = !stripled.powerState;
            if (stripled.powerState)
            {
                Stick.ColorSet(Stick.Color(255, 0, 0)); // to do pasarle las variables R G B
            }
            else
            {
                stripled.clear();
            }
        }
        else if (strcmp(action, "animation") == 0)
        {
            pattern ActivePattern = json["ActivePattern"];
            if (stripled.powerState)
            {
                Stick.ActivePattern = ActivePattern;
                Stick.Update();
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
    pinMode(STRIPLED_PIN, OUTPUT);

    Serial.begin(115200);
    delay(500);

    initSPIFFS();
    initWiFi();
    initWebSocket();
    initWebServer();

    Stick.begin();
    Stick.setBrightness(ledBrightness);
    Stick.clear();
    Stick.show();
}

// ----------------------------------------------------------------------------
// Main control loop
// ----------------------------------------------------------------------------

void loop()
{
    ws.cleanupClients();
    
    onboard_led.on = millis() % 1000 < 50;
    onboard_led.update();
}