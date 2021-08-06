# Introduction

The WiFi Gameboy Printer Emulator (wifi-gbp-emulator) is an easy-to-build easy-to-use tool to create digital copies of your Gameboy Camera photos. It is currently still under development but already reliable and usable; it's based on [ESP8266](https://github.com/esp8266/arduino) board design. This guide is aimed at beginners and will have some redundant information. 

This guide provides a basic step by step guide on how to get your wifi-gbp-emulator running. Generally the project is accessible and beginner friendly and required hardware is affordable and easy to obtain.

If you have any questions along the way or need help troubleshooting please refer to the [Gameboy Camera Club Discord Server](https://bit.ly/gbccd). 

## Hardware and tools

### Essential 
* Windows/Mac/Linux-PC
* Arduino IDE in a [current version](https://www.arduino.cc/en/software/) (1.8.x has been used successfully)
* ESP8266 Microcontroller
* Micro USB Cable
* Breakout Board
* GB Link Cable
* 3 Jumper Cables
* Multimeter

### Optional
* Soldering iron
* Perf-Board
* [OLED Screen](https://www.amazon.de/gp/product/B07BDFXFRK)

## Step 1: Flash the board

In this first step we will setup your computer to communicate with the ESP8266 Board and flash it. 

Required Tools for this step:
* PC/Laptop
* MicroUSB Cable
* ESP8266 Board

### 1.1 Install the Arduino IDE 
Download the Arduino IDE either from the Website (Windows/MacOS) from [here](https://www.arduino.cc/en/Main/Software) and follow the basic installation process.
 It is recommended for Linux Users to use your packet manager to install the IDE. 

### 1.2 Install ESP8266 in Arduino
Start the Arduino IDE and navigate to ```File -> Preferences``` and add the following line to "Additional Boards Manager URLs:"

````
http://arduino.esp8266.com/stable/package_esp8266com_index.json
````
Confirm with "OK" and navigate to ```Tools -> Board -> Board Manager```. In the filter mask search for ```esp8266``` and install the package. 

Lastly navigate to ```Tools -> Board``` and select your EPS8266 module

### 1.3 Install Arduino Libraries

Go to ``` Tools -> Manage Libraries``` and search in the filter mask for ArduinoJSON. Additional installation procedures can be found [here](https://arduinojson.org/v6/doc/installation/)

*Optionally,* if you are using an OLED display, use ``` Tools -> Manage Libraries ``` to search for and install the "Adafruit SSD1306" library.

### 1.4 Install arduino-esp8266fs-plugin

Download the latest version of the ```arduino-esp8266fs-plugin``` from [here](https://github.com/esp8266/arduino-esp8266fs-plugin/releases/)

In the Arduino IDE navigate to ```File -> Preferences``` and check for your ```Sketchbook location``` path. Navigate to that path in your file browser. 
Create a ```tools``` folder if there is not already one. Extract the contents of the archive into the ```tools``` folder, including all the intermediate folders. If done correctly, there should be a ```tools\ESP8266FS\tool\esp8266fs.jar``` file in your Arduino folder.

Restart the Arduino IDE after that.

### 1.5 Download the wifi-gbp-emulator github code and additional data

Download the full code from [here](https://github.com/HerrZatacke/wifi-gbp-emulator/archive/master.zip) as a ZIP file and extract it to your home directory. In your file browser navigate into the extracted folder and then into the ```wifi-gbp-emulator``` subfolder. Here create a copy of the ```config.h.sample.txt```. Rename the new file to ```config.h``` 

It is recommended to download the additional data for the GB-Printer-Web interface. To automatically download the additional data do:

#### Windows
Go to your Start Menu, type ```powershell``` and navigate to the directory you just extracted (e.g. cd ```YourUsername\Downloads\wifi-gbp-emulator\```)
Run the download script with ```.\update_w.ps1```

#### MacOS / Linux
Open your Terminal and navigate to the files you just extracted (e.g. ```cd ~/Downloads/wifi-gbp-emulator/```)
Type ```chmod +x update_w.sh```
Type ```./update_w.sh```

### 1.6 Upload the code and data

Connect your ESP8266 Board via USB to your PC. Navigate to ```File -> Open ...``` navigate to the folder you unzipped in the previous step. Open the ```wifi-gbp-emulator.ino``` 

To confirm that the code was downloaded correctly navigate to ```Sketch -> Verify/Compile```. Ensure at this point that the correct board is selected in ``` Tools -> Board ```. If everything works go to ```Sketch -> Upload``` and wait until the upload is finished.

Now nagivate to ```Tools -> ESP8266 Sketch Data Upload``` and wait until its finished.

### 1.7  Test the board

Unplug your ESP8266 Board from your PC and replug it. Open your WiFi Monitor and wait for it to finish scanning for available networks. You should see a wifi network named ```gameboyprinter``` with the password ```gameboyprinter```. 

After you connected to the WiFi open your browser and type in the URL ```http://gameboyprinter.local```. The Gameboy Printer Web Emulator should load. If it doesn't please try to connect to ```https://192.168.4.1``` instead. 

## Step 2: Preparing the Hardware

Now that you've got your ESP8266 running with the wifi-gbp-emulator it's time to connect your Gameboy to it to actually copy your images. There are non-destructive ways ([Example](https://github.com/Palmr/gb-link-cable)) but for ease of access we will have to cut up a link cable. You can either obtain those cheap from ebay or find reproductions on websites like AliExpress. 

Required tools for this step:
* Flashed ESP8266 Board
* Breakout Board
* Gameboy Link Cable
* 3 Jumper Cables
* Multimeter

Optional but recommended tools:
* Soldering Iron

### 2.1 Link Cable connection with a multimeter

#### 2.1.1 Preperation
Place your prepared ESP8266 Microcontroller on your breakout board.

Take your Link Cable and cut it in half. You will find six (6) thin wires inside. Carefully remove the cable shielding for 2cm / 1 inch close to where you cut the cable to expose the six wires. Now take your multimeter and check which wire has continuitiy to which port on the connector and write it down. 

#### 2.1.2 Preparing the connection wires 

We now will connect the wires to breakout board. Take your exposed connector and look at it from the front with the straight side up.

We will have to connect the ports 2-6 with jumper cables. Following is a diagram on how the ports are numerated:

```
________
| 6 4 2 |
\_5_3_1/

```
![Front view of Link Cable](/docs/link_cable_connector.jpg)

The easiest way to do that is by also cutting your jumper cables into half and remove a bit of isolation from five of those halfs. **It is recommended to solder both sides together**. Twirl the exposed ends together add solder. A visual example can be found here: [Youtube Link](https://www.youtube.com/watch?v=Zu3TYBs65FM)


### 2.2 Connecting your Gameboy to the ESP8266

We are now going to connect the modified link cable to the ESP8266. Connect the following cables to the holes on your breakout board next to the pins on the ESP8266 board:

```
________
| 6 4 2 |
\_5_3_1/

| Link Cable | ESP8266 |
|------------|---------|
| Pin 1      | D8      |
| Pin 2      | D7      |
| Pin 3      | D6      |
| Pin 5      | D5      |
| Pin 6      | G       |

```

## Step 3: Testing your Installation

Required tools for this step:
* Flashed ESP8266
* Breakout Board with modified Gameboy Link Cable
* PC/Laptop/Mobile Phone/Tablett/A fridge with a Browser (well, anything that can open Websites and show graphics. ;) )

Plug in your Link Cable to your Gameboy and start the ESP8266 though connecting it to a USB power source. Start your Gameboy with the Gameboy Camera inserted. Navigate to a picture you want to print and select ```Print```. You should see the ```Transferring``` screen and a short flashing of ```Printing``` -- You are done! Your wifi-gbp-emulator works successfully! _Tip: If you want to transfer all of your pictures right away press "Select" on your Gameboy in the main screen and use the Print Menu_

Disconnect the link cable and powercycle the ESP8266; connect to the wireless accesspoint and open the website as explained in Step 1.6. Change to the "Import" tab, click on "check printer" and you should should be able to import your pictures. From there on you can download pictures through the gallery, change the color palett (re-import afterwards).

Pat yourself on the back and have fun! Also make sure to join the [Gameboy Camera Club on Discord](https://bit.ly/gbccd) and share your pictures or participate in the Bi-Weekly challenges. 

## Optional Step 4: Soldering and adding the OLED Screen

### 4.1 Soldering

Required tools for this step:
* Flashed ESP8266 Board
* Perf-Board
* Soldering Iron

If you want to have a more permanent setup that you can take with you you can use a Perf-Board and solder the Link Cable wires directly to the corresponding pins.

### 4.2 OLED Screen

Required tools for this step:
* OLED Screen
* Flashed ESP8266 with modified Gameboy Link Cable either on a Breakout Board or Perf-Board
* PC with installed Arduino IDE

To check if your wifi-gbp-emulator is in Print or Web mode you can additionally connect an OLED Screen. Connect it to the following pins:

D1 -> Display SCL
D2 -> Display SDA

You will also need to modify your config.h and reupload it to your ESP8266. Remove the "//" from the line "// #define USE_OLED"

## FAQ / Troubleshooting

#### Printer Error #2
Your Link Cable is not connected correctly to the ESP8266. Check your cable connections.

#### Website only shows a 404 Error
You forgot to upload the additional data. Please Check Step 1.4 - 1.6

#### PermissionError "Access is Denied" while uploading sketch
The `arduino-esp8266fs-plugin` tool is not installed in the proper path. Please check Step 1.4.

### FAQ
**Q: How many pictures can I save on the ESP8266?**
**A:** 150. After that you should remove older pictures after backing them up to your PC.

**Q: Can I run the software I on the Gameboy WiFi printer I bought on Etsy?**
**A:** Yes, you can do that. Please be aware that this will remove the original software from the Gameboy WiFi printer. 

**Q: This is to complicated for me / My question isn't answered in the FAQ. Where can I find help?** 
**A:** You can ask any questions in the #development channel on the [Gameboy Camera Club Discord](https://bit.ly/gbccd)

**Q: Is there an easier option that doesn't require soldering?**
**A:** You can find a list of options [here](https://gameboymaniac.com/how-to-tranfer-photos-from-the-game-boy-camera-to-your-computer/) 
