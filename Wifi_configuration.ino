String ledState;
// time udp client
WiFiUDP Udp;
String RegisterWaarde;
String Register;
String Lengte;
String Type;


void Initialize_Wifi_And_Server() {
  Serial.println("Connecting to WiFi...");
  WiFi.hostname("marstekcontroller");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
    UpdateDisplay(1, "Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  UpdateDisplay(1, "Connected to WiFi...");
  delay(500);
  // turn display off to have a enough power from the master not to reset the board.
  DisplayOff();
  delay(500);
  // Print the ESP32's IP address
  Serial.print("ESP32 Web Server's IP address: ");
  Serial.println(WiFi.localIP());

  // // // Define a route to serve the HTML page
  //  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
  //    Serial.println("ESP32 Web Server: New request received:");  // for debugging
  //    Serial.println("GET /");        // for debugging
  //    request->send(200, "text/html", "<html><body><h1>Hello, ESP32!</h1></body></html>");
  //  });
  ConfigureWebServer();

  Serial.print("Server start ");
  // // Start the server
  server.begin();
}

void connectToWiFi() {
  int TryCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    TryCount++;
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    vTaskDelay(4000);
    if (TryCount == 10) {
      return;
    }
  }
}


void CollectTimeData() {
  Udp.begin(localPort);
  Serial.println("Getting time from time server");
  setSyncProvider(getNtpTime);
  setSyncInterval(86400);  // sync everyday
}

int currentDay() {
  return day();
}

int currentHour() {
  return hour();
}

time_t getNtpTime() {
  IPAddress ntpServerIP;  // NTP server's ip address

  while (Udp.parsePacket() > 0)
    ;  // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0;  // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;  // LI, Version, Mode
  packetBuffer[1] = 0;           // Stratum, or type of clock
  packetBuffer[2] = 6;           // Polling Interval
  packetBuffer[3] = 0xEC;        // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123);  //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}




void ConfigureWebServer() {
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", String(), false, indexprocessor);
  });

  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/style.css", "text/css");
  });

  // Route to load style.css file
  server.on("/batterijinfo.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/batterijinfo.html", String(), false, batteryprocessor);
  });


  server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request) {
    String message;
    if (request->hasParam("automode") && request->hasParam("load") && request->hasParam("active") && request->hasParam("button")) {
      String automode = request->getParam("automode")->value();
      String load = request->getParam("load")->value();
      String active = request->getParam("active")->value();
      String button = request->getParam("button")->value();
      message = "Received button: " + button + ", automode: " + automode + ", load: " + load + ", active: " + active;
      int hour = button.substring(6).toInt();
      bool automatic = false;
      if (automode == "Ja")
        automatic = true;
      bool enabled = false;
      if (active == "Ja")
        enabled = true;
      int power = load.toInt();
      // save the data
      updatebatteryconfiguration(hour, automatic, power, enabled);
      Serial.println(message);
    } else {
      message = "Missing parameters";
    }
    request->send(200, "text/plain", message);
  });
}

// Replaces placeholder with values
String indexprocessor(const String &var) {
  if (var.startsWith("kosten")) {
    int hour = var.substring(6).toInt();
    Serial.print("hour");
    Serial.println(hour);
    return GetPriceForHour(hour);
  }
  if (var.startsWith("load")) {
    int hour = var.substring(4).toInt();
    {
      int power = GetBatteryLoadHour(hour);
      return String(power);
    }
  }
  if (var.startsWith("autochecked")) {
    int hour = var.substring(11).toInt();
    {
      bool isAutomtic = GetAutomaticHour(hour);
      if (isAutomtic)
        return "checked";
      else
        return "";
    }
  }
  if (var.startsWith("active")) {
    int hour = var.substring(6).toInt();
    {
      bool isEnabled = GetEnabledHour(hour);
      if (isEnabled)
        return "checked";
      else
        return "";
    }
  }
  // return the input
  return String();
}

String batteryprocessor(const String &var) {
  String result = "";
  if (var.startsWith("Load")) {
    result = String(currentpower) + " Watt";
  } else if (var.startsWith("status")) {
    double capacity=(BatteryCapacity * batteryPercentage)/100.0;
    result = String(capacity) + " Watt";
  } else {
    result = GetBatteryInfoForPage(var);
  }
  return result;
}
