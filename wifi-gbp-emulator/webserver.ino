
WebServer server(80);
  
#define DUMP_CHUNK_SIZE 90

void defaultHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
}

void send404() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(404, "text/html", "<html><body><h1>404 - Not Found</h1><p>You probably forgot to upload the additional data.</p><br><a href=\"https://github.com/HerrZatacke/wifi-gbp-emulator/blob/master/beginner_setup_guide.md#14-install-arduino-esp8266fs-plugin\">Please Check Step 1.4 - 1.6</a></body></html>");
}

// delete all stored dumps
void clearDumps() {
  unsigned int dumpcount = 0;
  File dumpDir = FS.open("/d");    
  File file = dumpDir.openNextFile();

  char filename[12]; 

  while(file) {
    sprintf(filename, "/d/%s", file.name());
    dumpcount++;    
    file = dumpDir.openNextFile();
    FS.remove(filename);
  }
  
  char out[24];
  sprintf(out, "{\"deleted\":%d}", dumpcount);
  defaultHeaders();
  server.send(200, "application/json", out);
}

// serve list of saved dumps
void getDumpsList() {
  
  String fileName = "";
  String fileShort = "";

  // get number of files in /d/
  unsigned int dumpcount = 0;

  String out;
  String dumpList;
  bool sep = false;

  unsigned long total = 0;
  unsigned long used = 0;
  
  File dumpDir = FS.open("/d");
  //Random Values. Need find some way to get this values
  total = FS.totalBytes();
  used = FS.usedBytes();

  File file = dumpDir.openNextFile();
  while(file){
    dumpcount++;
    if (sep) {
      dumpList += ",";
    } else {
      sep = true;
    }    
    dumpList += "\"";
    dumpList += "/dumps/";
    dumpList += file.name();
    dumpList += "\"";
    file = dumpDir.openNextFile();
  }  
  
  unsigned long avail = total - used;
  
  char fs[100];
  sprintf(fs, "{\"total\":%d,\"used\":%d,\"available\":%d,\"maximages\":%d,\"dumpcount\":%d}", total, used, avail, MAX_IMAGES, dumpcount);

  defaultHeaders();
  server.send(200, "application/json", "{\"fs\":" + String(fs) + ",\"dumps\":[" + dumpList + "]}");
}

void getEnv() {
  char out[127];

  sprintf(out, "{\"version\":\"%s\",\"maximages\":%d,\"env\":\"%s\",\"fstype\":\"%s\",\"bootmode\":\"%s\",\"oled\":%s}",
    VERSION,
    MAX_IMAGES,
#ifdef ESP8266
    "esp8266",
#else
  #ifdef ESP32
    "esp8266",
  #else
    "unknown",
  #endif
#endif
#ifdef FSTYPE_LITTLEFS
    "littlefs",
#else
    "spiffs",
#endif
#ifdef SENSE_BOOT_MODE
    "5v-sense",
#else
    "alternating",
#endif
#ifdef USE_OLED
    "true"
#else
    "false"
#endif
  );

  defaultHeaders();
  server.send(200, "application/json", out);
}

void getConfig() {
  defaultHeaders();
  server.send(200, "application/json", wifiGetConfig());
}

void setConfig() {
  defaultHeaders();

  // Check if body received
  if (server.hasArg("plain") == false) {
    server.send(200, "application/json", JsonErrorResponse("empty request"));
    return;
  }

  server.send(200, "application/json", wifiSetConfig(server.arg("plain")));
}

// stream binary dump data to web-client
void handleDump() {
  String path = "/d/" + server.pathArg(0); 

  File file = FS.open(path); //check what print here
  Serial.println(path);
  if(!file || file.isDirectory()){
    Serial.println("failed to open file for reading");
    return;
  }
    
  if(file) {
    defaultHeaders();

    server.setContentLength(file.available() * 3);
    server.send(200, "text/plain");

    Serial.println(file.available());
    Serial.println(file.available() * 3);

    const char nibbleToCharLUT[] = "0123456789ABCDEF";

    char converted[DUMP_CHUNK_SIZE];
    uint8_t index = 0;

    while (file.available()) {
      char c = file.read();

      converted[index] = nibbleToCharLUT[(c>>4)&0xF];
      converted[index + 1] = nibbleToCharLUT[(c>>0)&0xF];
      converted[index + 2] = ' ';
      index += 3;

      if (index >= DUMP_CHUNK_SIZE || file.available() == 0) {
        Serial.println(index + 3);
        server.sendContent(converted, index);
        index = 0;
      }
    }
    file.close();
    return;
  }
  
  send404();
}

bool handleFileRead(String path) {
 
  path = "/w" + path;

  if (path.endsWith("/")) {
    path += "index.html";
  }

  String pathWithGz = path + ".gz";
  File file1 = FS.open(path);
  File file2 = FS.open(pathWithGz);
  
  if(file1 || file2) {
    String contentType = getContentType(path);

    if(file2) {
      path += ".gz";
    }
    
    file1.close();
    file2.close();  
    
    File file = FS.open(path);
    defaultHeaders();
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }

  Serial.print(path);
  Serial.println(" - Not Found");
  return false;
}

String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}

void webserver_setup() {
  server.on("/dumps/clear", clearDumps);
  server.on("/dumps/list", getDumpsList);
  server.on("/wificonfig/get", getConfig);
  server.on("/wificonfig/set", setConfig);
  server.on("/env.json", getEnv);

  #ifdef FSTYPE_LITTLEFS
    server.on(UriBraces("/dumps/{}"), handleDump);
  #else
    server.on(UriBraces("/dumps/d/{}"), handleDump);
  #endif

  server.onNotFound([]() {
    if (!handleFileRead(server.uri())) {
      send404();
    }
  });
  server.begin();
  Serial.println(F("Server started"));
}

void webserver_loop() {
  server.handleClient();
}
