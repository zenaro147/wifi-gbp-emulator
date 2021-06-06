
void fs_setup() {
  FS.begin();
  
  #ifdef ESP8266
    // clean a little harder
    Serial.println("Cleaning FS");
    for (int i = 0; i < 100; i++) {
      if (i % 10 == 0) {
        Serial.print(".");
      }
      FS.gc();
    }
    
    Dir dir = SPIFFS.openDir("/w");
    while (dir.next ()) {
      Serial.print(dir.fileSize());
      Serial.print("\t\t");
      Serial.println(dir.fileName());
    }
  #endif  
  #ifdef ESP32
    if (!FS.begin(true)) {
      Serial.println("LITTLEFS Mount Failed");
      return;
    }
  #endif
  Serial.println(" done");
}

int fs_info() {
  #ifdef ESP8266
    FSInfo fs_info;
    FS.info(fs_info);
    Serial.print("FILESYSTEM total: ");
    Serial.print(fs_info.totalBytes);
    Serial.print(" | used: ");
    Serial.println(fs_info.usedBytes);
  
    return (int)(
      (((float)fs_info.totalBytes - (float)fs_info.usedBytes) / (float)fs_info.totalBytes) * 100.0
    );
  #endif  
  #ifdef ESP32
    return(1);
  #endif
}

#ifndef SENSE_BOOT_MODE
bool fs_alternateBootMode() {
  String bootmode = "bootmode.txt";
  #ifdef ESP8266
    if (FS.exists(bootmode)) {
      FS.remove(bootmode);
      return false;
    } else {
      File file = FS.open(bootmode, "w");
      file.write("BOOT", 4);
      file.close();
      return true;
    }
  #endif  
  #ifdef ESP32
    char path[14];
    sprintf(path, "/%d", bootmode);
    File file = FS.open(path);
    bootmode="";
    if(file){
      while(file.available()){
        bootmode = bootmode + ((char)(file.read()));
      }
    }
    file.close();
    FS.remove(path);

    file = FS.open(path, FILE_WRITE);
    if (bootmode == "SERV"){
      file.print("PRNT");
      file.close();    
      return false;
    }else{
      file.print("SERV");
      file.close();    
      return true;
    }
  #endif
}
#endif
