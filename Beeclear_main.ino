const char* beeclearUrl = "http://beeclear/bc_current";

/* if not authenticated run following command from browser*/
/* http://beeclear/bc_security?set=off */

void GetCurrentLoad() {
  HTTPClient http;

  // Your Domain name with URL path or IP address with path
  http.begin(beeclearUrl);
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
  power=doc["verbruik0"];
  if(power==-1)
  {
    power=doc["verbruil1"];
  }
  returnpower=doc["leveren0"];
  if(returnpower==-1)
  {
  returnpower=doc["leveren1"];
  }
  currentLoad=power-returnpower;

 //Serial.println("payload Total: " + String(currentpower) + " power: " + String(power)+ " returnpower: "+ String(returnpower) );
  
}