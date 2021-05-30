# WiFi GBP Emulator
A GameBoy printer emulator which provides the received data over a WiFi connection  
This code has been created for a "LOLIN(WEMOS) D1 R2 & mini" [ESP8266 based board](https://github.com/esp8266/arduino) and adapted for a "DOIT ESP32 DEVKIT V1" [Arduino core for the ESP32](https://github.com/espressif/arduino-esp32)

This is (for now) a Prototybe branch to use the original code into the ESP32 (and maybe get some advantages from his hardware, like the dual core)

# Known Bugs
ESP32 Ony:

*WiFi doesn't connect (maybe sometimes?)

*Batch Print from Gameboy Camera stop writing files after a while, but the data stream doen't stop (maybe the FileSystem is based on LittleFS?)

*Access Point didn't initialize properly
