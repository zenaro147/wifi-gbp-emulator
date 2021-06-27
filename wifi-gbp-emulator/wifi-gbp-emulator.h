#include "config.h"
#include <ArduinoJson.h>

#include "FS.h"
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#include <uri/UriBraces.h>

#ifndef DEFAULT_AP_SSID
#define DEFAULT_AP_SSID "gameboyprinter"
#endif
#ifndef DEFAULT_AP_PSK
#define DEFAULT_AP_PSK "gameboyprinter"
#endif
#ifndef DEFAULT_MDNS_NAME
#define DEFAULT_MDNS_NAME "gameboyprinter"
#endif
#ifndef WIFI_CONNECT_TIMEOUT
#define WIFI_CONNECT_TIMEOUT 10000
#endif
#ifndef MAX_IMAGES
#define MAX_IMAGES 150
#endif
#ifndef WIFI_BLINK_DELAY
#define WIFI_BLINK_DELAY 2000
#endif
#ifndef LED_BLINK_PIN
#define LED_BLINK_PIN 2
#endif
#ifndef GB_5V_OUT
#define GB_5V_OUT 5
#endif
#ifndef GB_MISO
#define GB_MISO 12
#endif
#ifndef GB_MOSI
#define GB_MOSI 13
#endif
#ifndef GB_SCLK
#define GB_SCLK 14
#endif


#include <LITTLEFS.h>
#define FS LITTLEFS

#define MODE_PRINT true
#define MODE_SERVE false
