const char* homewizardP1Url = "http://HW-p1meter-3CB898/api/v1/data";

/* if not authenticated run following command from browser*/
/* http://beeclear/bc_security?set=off */

void GetCurrentLoad() {
  HTTPClient http;

  // Your Domain name with URL path or IP address with path
  http.begin(homewizardP1Url);
  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    // Serial.print("HTTP Response code: ");
    // Serial.println(httpResponseCode);
    String payload = http.getString();
    ProcessJsonData(payload);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();
}


void ProcessJsonData(String payload) {
  Serial.println(payload);
  JsonDocument doc;

  deserializeJson(doc, payload);
  power=doc["active_power_w"];
  
  currentLoad=power;

 //Serial.println("payload Total: " + String(currentpower) + " power: " + String(power)+ " returnpower: "+ String(returnpower) );
  
}