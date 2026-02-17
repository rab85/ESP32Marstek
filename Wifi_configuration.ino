String ledState;

String RegisterWaarde;
String Register;
String Lengte;
String Type;



void Initialize_Wifi_And_Server() {
  // setup time and time zone settings


  Serial.println("Connecting to WiFi..." + String(ssid));
  WiFi.hostname("marstekcontroller");
  WiFi.begin(ssid, password);
  String extend = "";
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    UpdateDisplay(1, "Connecting to WiFi..." + extend);
    extend += ".";
  }
  Serial.println("Connected to WiFi");
  UpdateDisplay(1, "Connected to WiFi...");
  delay(500);

  // Print the ESP32's IP address
  Serial.print("ESP32 Web Server's IP address: ");
  Serial.println(WiFi.localIP());

  ConfigureWebServer();

  Serial.print("Server start ");
  // // Start the server
  server.begin();
}

void connectToWiFi() {
  int TryCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    TryCount++;
    Serial.println("Connecting to WiFi..." + String(ssid));
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    vTaskDelay(4000);
    if (TryCount == 10) {
      return;
    }
  }
}

int currentDay() {
  return timeinfo.tm_mday;
}

int currentHour() {
  return timeinfo.tm_hour;
}

int weekDay() {
  return timeinfo.tm_wday;
}

void FillLocalTime() {
  time(&now);
  localtime_r(&now, &timeinfo);
}


void setTimezone(String timezone) {
  long dateinepoc;
  Serial.printf("  Setting Timezone to %s\n", timezone.c_str());
  setenv("TZ", timezone.c_str(), 1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
  // FillLocalTime();

  // Serial.print(timeinfo.tm_hour);
  // Serial.print(":");
  // Serial.print(timeinfo.tm_min);
  // Serial.print(":");
  // Serial.print(timeinfo.tm_sec);
  // Serial.print("   ");
  // Serial.print(timeinfo.tm_mday);
  // Serial.print("-");
  // Serial.print(timeinfo.tm_mon);
  // Serial.print("-");
  // Serial.println(timeinfo.tm_year);
}

void initTime(String timezone) {
  struct tm timeinfo;

  Serial.println("Setting up time");
  configTime(0, 0, MY_NTP_SERVER);  // First connect to NTP server, with 0 TZ offset
  if (!getLocalTime(&timeinfo)) {
    Serial.println("  Failed to obtain time");
    return;
  }
  Serial.println("  Got the time from NTP");
  // Now we can set the real timezone
  setTimezone(timezone);

  //setTime(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, timeinfo.tm_mday, timeinfo.tm_mon, timeinfo.tm_year);
}


void ConfigureWebServer() {

  server.on("/status", HTTP_GET, handleGetStatus);
  server.on("/data", HTTP_POST, handlePostRequest, NULL, handlePostBody);


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
    result = String(currentLoad) + " Watt";
  } else if (var.startsWith("status")) {
    double capacity = (BatteryCapacity * batteryPercentage) / 100.0;
    result = String(capacity) + " Watt";
  } else if (var.startsWith("energieprijs")) {
    result = String(GetCurrentPrice() + " ct l:" + String(GetPriceLevel()));
  } else {
    result = GetBatteryInfoForPage(var);
  }
  return result;
}

void handleGetStatus(AsyncWebServerRequest *request) {
  StaticJsonDocument<200> doc;
  doc["status"] = "OK";
  doc["uptime_ms"] = millis();
  doc["load"] = GetCurrentBatteyPower();

  String response;
  serializeJson(doc, response);

  request->send(200, "application/json", response);
}

void handlePostRequest(AsyncWebServerRequest *request) {
  // this is a dummy function to keep the compiler happy
}

void handlePostBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  String body = "";
  for (size_t i = 0; i < len; i++) {
    body += (char)data[i];
  }
  if (body == "") {
    request->send(400, "application/json", "{\"error\":\"No body received\"}");
    return;
  }

  Serial.println(body);

  // Parse incoming JSON
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, body);

  if (error) {
    request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  // Example: read a field from JSON
  const char *requestLoad = doc["load"] | "";
  if (strcmp(requestLoad, "") == 0) {
    request->send(400, "application/json", "{\"error\":\"Load not specified\"}");
    return;
  }

  int requested = String(requestLoad).toInt();

  bool result = ExternalControlBattery(requested);
  if (result) {
    StaticJsonDocument<200> res;
    res["requested"] = requested;
    String response;
    serializeJson(res, response);
    request->send(200, "application/json", response);
  }
  // not allowed
  request->send(409, "application/json", "");
}
