
void fs_setup() {
  #ifdef FSTYPE_SDCARD
    spiSD.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS); //SCK,MISO,MOSI,SS //HSPI1
    FSYS.begin(SD_CS, spiSD);
  #else
    FSYS.begin();
  #endif 
  if (!FSYS.begin(true)) {
    Serial.println("LITTLEFS Mount Failed");
    return;
  }

  #ifdef FSTYPE_SDCARD
    uint8_t cardType = SD.cardType();
    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        #ifdef USE_OLED
          oled_msg("No SD Card","Rebooting...");
          delay(3000);
        #endif
        ESP.restart();
        return;
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

  Serial.println(" done");
}



void fs_info() {
  uint64_t totalBytes=0;
  uint64_t usedBytes=0;
  totalBytes = FSYS.totalBytes();
  usedBytes = FSYS.usedBytes();   
  
  Serial.print("FILESYSTEM total: ");
  Serial.print(totalBytes);
  Serial.print(" | used: ");
  Serial.println(usedBytes);
 
}

#ifndef SENSE_BOOT_MODE
bool fs_alternateBootMode() {
  String bootmode = "bootmode.txt";
  char path[14];
  
  sprintf(path, "/%s", bootmode);
  
  if(SD.remove(path)){
    return false;
  } else {
    File file = SD.open(path,"w");
    file.print("BOOT");
    file.close();
    return true;
  } 
}
#endif
