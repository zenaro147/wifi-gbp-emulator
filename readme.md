# WiFi GBP Emulator
A GameBoy printer emulator which provides the received data over a WiFi connection  
This code has been originally created for a "LOLIN(WEMOS) D1 R2 & mini" [ESP8266 based board](https://github.com/esp8266/arduino) and now adapted for a "DOIT ESP32 DEVKIT V1" [ESP32 based board](https://github.com/espressif/arduino-esp32)

If you have a ESP8266, I recommend to check the original project from [HerrZatacke](https://github.com/HerrZatacke/wifi-gbp-emulator)

The required (recommended) [gameboy printer web interface can be found on github as well](https://github.com/HerrZatacke/gb-printer-web/)  

## Setup
For uploading the Filesystem to the ESP you require the [Arduino ESP32 filesystem uploader](https://github.com/lorol/arduino-esp32fs-plugin). Follow the installation instructions on the repository.
Before compiling the project you need to create a `config.h` inside the project folder  
([`config.h.sample.txt`](/wifi-gbp-emulator/config.h.sample.txt) should be used as a reference)    
I recommended to adjust the parameters there to match with your board 

## Bootmode
The default will alternate the bootmode automatically between printer and server on each boot, so you can use the reset-button to switch modes  

## Configuration
If the device is not configured the default settings (AP only) will be used

### Default config
If not present the ESP will set up an accesspoint with ssid/password `gameboyprinter`  
It will output it's IP adress over the serial monitor and on an optionally connected display  
(usually the device should be accessible via `http://192.168.4.1`)  
It will also be discoverable via Bonjour/mDNS via `http://gameboyprinter.local`  

### Manual Setup via `conf.json`
Wifi setup can be done via a file `conf.json` in the `/data/` folder  
The basic format are as follows:
```` json
{
  "mdns": "gameboyprinter",
  "ap": {
    "ssid":"gameboyprinter",
    "psk":"gameboyprinter"
  },
  "networks": [
    {
      "ssid":"Your SSID",
      "psk":"Your wifi password"
    }
  ]
}
````

All config options are optional

#### `"mdns"`
Change the name by which the printer will be accessible via Bonjour/mDNS

#### `"ap"`
AccessPoint config  
Enter ssid/psk to open an accesspoint different to the default one.

#### `"networks"`
Array of objects with keys ssid/psk  
The first reachable network in that list will be used to conect to  
⚠ Do not use more than 10 elements. Otherwise you might see an "Out of Memory" error in the serial monitor  

### Manual Setup via web interface
The following will describe the "raw" interface for updating ther configuration. You will only need this if you decide not to use the existing web-interface

#### Reading
calls to `/wificonfig/get` delivers the actual config (passwords are omitted)

#### Writing
calls to `/wificonfig/set` will update the sent properties  
make sure to send the data as a JSON string in the POST-Body

##### Updating existing networks
* Networks are based on the `ssid`-property
* Existing networks do not have to be sent if they should remain unchanged.
* If the update for a network contains `"delete": true`, the networek will be deleted
* If the update for a network contains a `psk` it will be updated otherwise left untouched

## Automatic update of the webserver content
* You can run [`update_w.ps1`](./update_w.ps1) (windows-powershell) or [`update_w.sh`](./update_w.sh) (bash) to download the latest release of the [gb-printer-web](https://github.com/HerrZatacke/gb-printer-web/releases/) and automatically unzip it to the correct folder  
* After doing so upload the content of that folder to the ESP via the [Arduino ESP32 filesystem uploader](https://github.com/lorol/arduino-esp32fs-plugin) in the Arduino IDE or copy to your SD Card.

## Hardware Setup
Gameboy Original/Color Link Cable Pinout
```
 __________
|  6  4  2 |
 \_5__3__1_/ (at cable)

| Link Cable |  ESP32  |
|------------|---------|
| Pin 1      | Any pin | 
| Pin 2      | G19     |
| Pin 3      | G23     |
| Pin 5      | G18     |
| Pin 6      | G       |

```
Additionally an [OLED Display](https://github.com/HerrZatacke/wifi-gbp-emulator/#oled-display) can be added via G22 -> Display SCL / G21 -> Display SDA. If you have some trouble with these pins, you can use any other GPIO pin as SCL/SDA

## Micro SD Card Setup
You can add a [Micro SD Card like this](https://www.amazon.com/Adapter-Reader-interface-driver-Arduino/dp/B01MSNX0TW/) to save the dumps and the web server content.
To use it, you need to uncomment `#define FSTYPE_SDCARD` and connect the pins following this schema
```
| SD ADAPTER |  ESP32  |
|------------|---------|
| CS         | G15     | 
| SCK        | G14     |
| MISO       | G27     | <-- DON'T USE THE G12... YOU CAN USE ANY OTHER PIN AVAILABLE
| MOSI       | G13     |
| GND        | G       |
| VCC        | V5      |

```
Instead use the [Arduino ESP32 filesystem uploader](https://github.com/lorol/arduino-esp32fs-plugin), just format your SD Card to `FAT32` and copy de content of `DATA` folder to the root of your SD card

## OLED Display
You can add a [tiny oled display like this](https://www.amazon.de/gp/product/B07BDFXFRK) or [(probably) this](https://de.aliexpress.com/item/32672229793.html)   
To use it, you need to uncomment `#define USE_OLED` and the following lines   
The display will show the current wifi-config while in server mode, as well as the number of printed images  when in printer mode  

## Links
* Original GPB-Emulator by [mofosyne: Arduino Gameboy Printer Emulator](https://github.com/mofosyne/arduino-gameboy-printer-emulator)  
* Original WiFi GBP Emulator by [HerrZatacke: WiFi GBP Emulator](https://github.com/HerrZatacke/wifi-gbp-emulator)

## Known Bugs
* WifiMulti doesn't work properly. Only Access Point works or using a single Wifi Network;
* Only 5 games don't have support yet. You can check the compatibility list here: [Game Boy Printer Emulator - Games Support](https://docs.google.com/spreadsheets/d/1RQeTHemyEQnWHbKEhUy16cPxR6vA3YfeBbyx2tIXWaU/edit#gid=0) 
