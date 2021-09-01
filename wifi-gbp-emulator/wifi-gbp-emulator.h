#include "config.h"
#include <ArduinoJson.h>

#include "FS.h"
#include <WiFi.h>
//#include <WiFiMulti.h>
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
  #ifdef FSTYPE_SDCARD
    #define MAX_IMAGES 400
  #else
    #define MAX_IMAGES 150
  #endif  
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
#define GB_MISO 19
#endif
#ifndef GB_MOSI
#define GB_MOSI 23
#endif
#ifndef GB_SCLK
#define GB_SCLK 18
#endif

#ifdef FSTYPE_SDCARD
  #include "SD.h"
  #include "SPI.h"
  #define FSYS SD
  
  #ifndef SD_CS 
    #define SD_CS 15
  #endif
  #ifndef SD_SCK
    #define SD_SCK 14
  #endif
  #ifndef SD_MOSI
    #define SD_MOSI 13
  #endif
  #ifndef SD_MISO
    #define SD_MISO 27
  #endif

#else
  #include <LittleFS.h>
  #define FSYS LittleFS
#endif

#define MODE_PRINT true
#define MODE_SERVE false
