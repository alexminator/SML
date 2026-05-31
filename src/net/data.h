// WiFi credentials - set via build flags from .env file
// For development: create .env file with WIFI_SSID and WIFI_PASS
#ifndef DEFAULT_WIFI_SSID
#define DEFAULT_WIFI_SSID ""
#endif
#ifndef DEFAULT_WIFI_PASS
#define DEFAULT_WIFI_PASS ""
#endif

// Solo declaraciones — definiciones en main.cpp
extern const char *WIFI_SSID;
extern const char *WIFI_PASS;
extern const char *WEB_NAME;

