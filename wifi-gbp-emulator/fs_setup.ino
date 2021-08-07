
bool fs_setup() {
  #ifdef FSTYPE_SDCARD
    spiSD.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS); //SCK,MISO,MOSI,SS //HSPI1
    FSYS.begin(SD_CS, spiSD);
  #else
    FSYS.begin();
  #endif 
  if (!FSYS.begin(true)) {
    #ifdef FSTYPE_SDCARD
      Serial.println("SD Card Mount Failed");
    #else
      Serial.println("LittleFS Mount Failed");
    #endif
    #ifdef USE_OLED
      oled_msg("ERROR", "Can't init FileSystem");
    #endif
    return false;
  }else{
    #ifdef FSTYPE_SDCARD
      uint8_t cardType = SD.cardType();
      if(cardType == CARD_NONE){
          Serial.println("No SD card attached");
          #ifdef USE_OLED
            oled_msg("No SD Card","Rebooting...");
          #endif
          delay(3000);
          ESP.restart();
      }
    
      Serial.print("SD Card Type: ");
      if(cardType == CARD_MMC){
          Serial.println("MMC");
      } else if(cardType == CARD_SD){
          Serial.println("SDSC");
      } else if(cardType == CARD_SDHC){
          Serial.println("SDHC");
      } else {
          Serial.println("UNKNOWN");
      }
    
      uint64_t cardSize = SD.cardSize() / (1024 * 1024);
      Serial.printf("SD Card Size: %lluMB\n", cardSize);
    #endif

    File root = FSYS.open("/t");
    if(!root){
        Serial.println("- failed to open Temp directory");
        if(FSYS.mkdir("/t")){
            Serial.println("Temp Dir created");
        } else {
            Serial.println("mkdir failed");
        }
    }else{
      Serial.println("Temp folder already exist.");
    }
  
    root = FSYS.open("/d");
    if(!root){
        Serial.println("- failed to open Dump directory");
        if(FSYS.mkdir("/d")){
            Serial.println("Dump Dir created");
        } else {
            Serial.println("mkdir failed");
        }
    }else{
      Serial.println("Dump folder already exist.");
    }

    for (int i = 0; i < 100; i++) {
      if (i % 10 == 0) {
        Serial.print(".");
      }
    }
    Serial.println(" done"); 
    
    return true;
  }
}


void fs_info() {
  uint64_t totalBytes=0;
  uint64_t usedBytes=0;
  totalBytes = FSYS.totalBytes();
  usedBytes = FSYS.usedBytes();   
  
//  Serial.print("FILESYSTEM total: ");
//  Serial.print(totalBytes);
//  Serial.print(" | used: ");
//  Serial.println(usedBytes);
 
}

#ifndef SENSE_BOOT_MODE
bool fs_alternateBootMode() {
  String bootmode = "bootmode.txt";
  char path[14];  
  sprintf(path, "/%s", bootmode);
  
  if(FSYS.remove(path)){
    return false;
  } else {
    File file = FSYS.open(path,FILE_WRITE);
    file.print("BOOT");
    file.close();
    return true;
  } 
}
#endif


#ifndef SENSE_BOOT_MODE
bool fs_checkBootFile() {
  String bootmode = "bootmode.txt";
  char path[14];  
  sprintf(path, "/%s", bootmode);

  File file = FSYS.open(path);
  if(file){
    return false;
  } else {
    return true;
  }
}
#endif
