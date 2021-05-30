
void mdns_setup() {
  String protocol = F("http://");
  String ip = "";
  String local = F(".local");

  
  #ifdef ESP8266
    if (!MDNS.begin(mdnsName)) {
      Serial.println("Error setting up MDNS responder!");
    }
  #endif  
  #ifdef ESP32
    if (!MDNS.begin(DEFAULT_MDNS_NAME)) {
      Serial.println("Error setting up MDNS responder!");
    }
  #endif
  

  MDNS.addService("http", "tcp", 80);
  WiFi.hostname(mdnsName + local);

  Serial.println("mDNS responder started");

  if (hasNetworkSettings) {
    ip = WiFi.localIP().toString();
  } else {
    ip = WiFi.softAPIP().toString();
  }

  Serial.println(protocol + ip);
  Serial.println(protocol + mdnsName);
  Serial.println(protocol + mdnsName + local);

  #ifdef USE_OLED
  showWifiStats(ip, mdnsName + local);
  #endif
}

void mdns_loop() {
  #ifdef ESP8266
    MDNS.update();
  #endif
}
