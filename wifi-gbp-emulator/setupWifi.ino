
  #ifdef ESP8266
    ESP8266WiFiMulti wifiMulti;
  #endif  
  #ifdef ESP32
    WiFiMulti wifiMulti;
  #endif

void createEmptyConfig() {
  Serial.println("Preparing empty conf.json. \nYou can configure WiFi-Settings via the web interface.");
  File confFileEmpty = FS.open("/conf.json", "w");
  confFileEmpty.println("{}");
  confFileEmpty.close();
}

void setupWifi() {
  StaticJsonDocument<1023> conf;  
  #ifdef ESP8266
    File confFile = FS.open("/conf.json", "r");
  #endif  
  #ifdef ESP32
    File confFile = FS.open("/conf.json");
  #endif

  if (confFile) {
    DeserializationError error = deserializeJson(conf, confFile.readString());
    confFile.close();

    if (!error) {
      if (conf.containsKey("mdns")) {
        if (String(conf["mdns"].as<String>()) != "") {
          mdnsName = String(conf["mdns"].as<String>());
        }
      }

      if (conf.containsKey("networks")) {
        JsonArray networks = conf["networks"].as<JsonArray>();
        for(JsonVariant networkSetting : networks) {
          const char *ssid = networkSetting["ssid"].as<const char*>();
          const char *password = networkSetting["psk"].as<const char*>();
          if (ssid != "null" && ssid != "" && password != "") {
            #ifdef ESP8266
              wifiMulti.addAP(ssid, password);
            #endif  
            #ifdef ESP32
              if (!hasNetworkSettings){
                WiFi.begin(ssid, password);
                //wifiMulti.addAP(ssid, password);
              }
            #endif
            hasNetworkSettings = true;
          }
        }
      }

      if (conf.containsKey("ap")) {
        accesPointSSID = String(conf["ap"]["ssid"].as<String>());
        accesPointPassword = String(conf["ap"]["psk"].as<String>());

        if (accesPointSSID == "null" || accesPointSSID == "" || accesPointPassword == "") {
          accesPointSSID = DEFAULT_AP_SSID;
          accesPointPassword = DEFAULT_AP_PSK;
        }
      } else {
        Serial.println("No AccessPoint settings configured - using default");
      }
    } else {
      Serial.println("Error parsing conf.json");
      Serial.println(error.c_str());
      createEmptyConfig();
    }
  } else {
    Serial.println("Could not open conf.json");
    createEmptyConfig();
  }

  conf.clear();

  // Connect to existing WiFi
  if (hasNetworkSettings) {
    Serial.print("Connecting to wifi ");  
    #ifdef ESP8266
      WiFi.mode(WIFI_STA);
    #endif  
    #ifdef ESP32
      WiFi.mode(WIFI_MODE_STA);
    #endif

    bool connectionBlink = false;
    unsigned int connTimeout = millis() + WIFI_CONNECT_TIMEOUT;
    unsigned int connTick = 0;

    #ifdef ESP8266
       while (hasNetworkSettings && (wifiMulti.run() != WL_CONNECTED)) {
        delay(250);
        digitalWrite(LED_BLINK_PIN, connectionBlink);
        connectionBlink = !connectionBlink;
        connTick++;
  
        unsigned int remain = connTimeout - millis();
        if (remain <= 0 || remain > WIFI_CONNECT_TIMEOUT) {
          Serial.println("WiFi Connection timeout - starting AccesPoint");
          hasNetworkSettings = false;
        }
  
        if (connTick % 4 == 0) {
          Serial.print(".");
          #ifdef USE_OLED
          oled_msg("Connecting to wifi...", String(remain / 1000) + "s");
          #endif
        }
      }
    #endif  
    #ifdef ESP32
       while (hasNetworkSettings && (WiFi.status() != WL_CONNECTED)) {
       //while (hasNetworkSettings && (wifiMulti.run() != WL_CONNECTED)) {
        delay(250);
        digitalWrite(LED_BLINK_PIN, connectionBlink);
        connectionBlink = !connectionBlink;
        connTick++;
  
        unsigned int remain = connTimeout - millis();
        if (remain <= 0 || remain > WIFI_CONNECT_TIMEOUT) {
          Serial.println("WiFi Connection timeout - starting AccesPoint");
          hasNetworkSettings = false;
        }
  
        if (connTick % 4 == 0) {
          Serial.print(".");
          #ifdef USE_OLED
          oled_msg("Connecting to wifi...", String(remain / 1000) + "s");
          #endif
        }
      }
    #endif
    
  }

  // will be false if no connection to any ssid could be made
  if (hasNetworkSettings) {
    Serial.print("\nConnected to ");
    Serial.print(WiFi.SSID());
    Serial.print(" with IP address: ");
    Serial.println(WiFi.localIP());
    return;
  } else {

    #ifdef ESP8266
      WiFi.mode(WIFI_AP);
    #endif  
    #ifdef ESP32
      WiFi.mode(WIFI_MODE_AP);
    #endif
    
    #ifdef ESP8266
      WiFi.softAP(accesPointSSID, accesPointPassword);
    #endif
    #ifdef ESP32
      const char * accesPointSSIDc = accesPointSSID.c_str();
      const char * accesPointPasswordc = accesPointPassword.c_str();
      
      WiFi.softAP(accesPointSSIDc, accesPointPasswordc);
    #endif
    Serial.println("AccessPoint " + accesPointSSID + " started");
  }
}

#ifdef USE_OLED
void showWifiStats(String ip, String mdnsName) {
  String protocolShort = F("IP: ");

  if(hasNetworkSettings) {
    oled_msg(
      "Connected to WiFi",
      "SSID: " + WiFi.SSID(),
      protocolShort + ip,
      mdnsName
    );
  } else {
    oled_msg(
      "AP: " + accesPointSSID,
      "PW: " + accesPointPassword,
      protocolShort + ip,
      mdnsName
    );
  }

  oled_drawIcon();
}
#endif
