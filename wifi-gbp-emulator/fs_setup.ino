
void fs_setup() {
  FS.begin();
  if (!FS.begin(true)) {
    Serial.println("LITTLEFS Mount Failed");
    return;
  }
  Serial.println(" done");
}

int fs_info() {
  unsigned int totalBytes=0;
  unsigned int usedBytes=0;
  totalBytes = FS.totalBytes();
  usedBytes = FS.usedBytes();   
  
  Serial.print("FILESYSTEM total: ");
  Serial.print(totalBytes);
  Serial.print(" | used: ");
  Serial.println(usedBytes);
  
    return (int)(
      (((float)totalBytes - (float)usedBytes) / (float)totalBytes) * 100.0
    );
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
