
// LittleFS _seems_ 100% slower than SPIFFS in this use-case
// enable only if you think you know what you are doing
// #define FSTYPE_LITTLEFS

// Alternate boot mode lets you toggle the mode by using the reset switch
// If commented out, the signal on pin GB_5V_OUT is being used to determine mode
// #define ALTERNATE_BOOT_MODE
// #define GB_5V_OUT 5

// Number of dumps after which the printer reports to be full.
// due to performance reasons, there should always remain some free space on the Filesystem
// #define MAX_IMAGES 150

// Uncomment if using an adafruit oled display
// check oled.ino to define pins
// #define USE_OLED
// Alternative OLED Pins
// #define OLED_SDA 0
// #define OLED_SCL 4
// you can invert the display to easily spot the actual dimensions
// #define OLED_INVERT

// You can override the defaults for AccesPoint SSIF/PASS/MDNSNAME
// All those parameters can be configured within /data/conf.json
// #define DEFAULT_AP_SSID "gameboyprinter"
// #define DEFAULT_AP_PSK "gameboyprinter"
// #define DEFAULT_MDNS_NAME "gameboyprinter"

// Just the delay between the WiFi-Mode "Pings" of the LED
// #define WIFI_BLINK_DELAY 5000

// Define different PINS depending on your hardware
// #define LED_BLINK_PIN 2
// #define GB_MISO 12
// #define GB_MOSI 13
// #define GB_SCLK 14
