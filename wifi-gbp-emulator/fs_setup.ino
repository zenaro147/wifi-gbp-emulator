
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

  file = FS.open(path, "w");
  if (bootmode == "SERV"){
    file.print("PRNT");
    file.close();    
    return false;
  }else{
    file.print("SERV");
    file.close();    
    return true;
  }
}
#endif
