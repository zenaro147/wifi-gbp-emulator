#include "wifi-gbp-emulator.h"
#define VERSION "0.3.5-custom"

#ifdef FSTYPE_SDCARD
  SPIClass spiSD(HSPI);
#endif

// Variables used across multiple files, so they need to be defined here
String mdnsName = DEFAULT_MDNS_NAME;
String accesPointSSID = DEFAULT_AP_SSID;
String accesPointPassword = DEFAULT_AP_PSK;
bool hasNetworkSettings = false;
bool bootMode;
bool isFileSystemMounted = false;
bool isWriting = false;
unsigned int totalMultiImages = 1;


#ifdef BUTTON_FEATURE 
  // Button Variables
  long buttonTimer = 0;
  long longPressTime = 2000;
  boolean buttonActive = false;
  boolean longPressActive = false;
#endif

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n\n\n");

  #ifdef USE_OLED
    oled_setup();
    delay(5000);
  #endif
  
  isFileSystemMounted = fs_setup();
  if(isFileSystemMounted){   
    /* Pin for pushbutton */ 
    #ifdef BUTTON_FEATURE 
      pinMode(BTN_PUSH, INPUT);
    #endif
    
    #ifdef USE_OLED
      oled_bootmessages();
    #endif
  
    WiFi.disconnect();
    delay(1000);
  
    pinMode(GB_5V_OUT, INPUT);
    pinMode(LED_BLINK_PIN, OUTPUT);
  
    #ifdef SENSE_BOOT_MODE
      bootMode = digitalRead(GB_5V_OUT);
    #else
      bootMode = fs_alternateBootMode();
    #endif
  
    Serial.println((String)"\n\nv" + VERSION);
    if (bootMode == MODE_PRINT) {
      Serial.println("-----------------------");
      Serial.println("Booting in printer mode");
      Serial.println("-----------------------\n");
      digitalWrite(LED_BLINK_PIN, false);
      espprinter_setup();
      #ifdef USE_OLED
        showPrinterStats();
      #endif
    } else {  
      Serial.println("-----------------------");
      Serial.println("Booting in server mode");
      Serial.println("-----------------------\n");
      digitalWrite(LED_BLINK_PIN, true);
      setupWifi();
      mdns_setup();
      webserver_setup();
      wifi_blink_setup();
    }
  }
}

void loop() {
  if(isFileSystemMounted){
    if (bootMode == MODE_SERVE) {
      wifi_blink_loop();
      webserver_loop();
      mdns_loop();
    } else {
      espprinter_loop();
    }
  
    #ifdef SENSE_BOOT_MODE
    if (bootMode != digitalRead(GB_5V_OUT)) {
      ESP.restart();
    }
    #endif    
  }

  #ifdef BUTTON_FEATURE 
    // Feature to detect a short press and a Long Press
    if(!isWriting){
      if (digitalRead(BTN_PUSH) == HIGH) {  
        if (buttonActive == false) {  
          buttonActive = true;
          buttonTimer = millis();  
        }  
        if ((millis() - buttonTimer > longPressTime) && (longPressActive == false)) {  
          longPressActive = true;
          Serial.println("Rebooting...");
          ESP.restart();
        }  
      } else {  
        if (buttonActive == true) {
          if (longPressActive == true) {
            longPressActive = false;  
          } else {
            if(isFileSystemMounted){
              if((totalMultiImages-1) > 1){
                Serial.println("Force File Merger");
                #ifdef USE_OLED
                  oled_msg("Force Merging Files...");
                #endif
                isWriting = true;
                totalMultiImages--;
                callFileMerger();
            //    gpb_mergeMultiPrint(); 
              }
            }
          }  
          buttonActive = false;  
        }  
      }
    }
  #endif
}
