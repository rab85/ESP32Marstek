void Initialize_Display() {

  Wire.begin(SDA_PIN, SCL_PIN);
  pixels.begin();


  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }

  display.display();
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);       // Normal 1:1 pixel scale
  display.setTextColor(WHITE);  // Draw white text
  display.setRotation(ROTATION);
  display.display();
}

void DisplayInfo() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(PrintTime());
  display.println("Verbruik: " + String(currentpower) + " W");
  display.println("Battery : " + String(-currentBatteryPower) + " W");
  display.println("Berekend : " + String(selectedPower) + " W");
  display.println("Beschikbaar: " + String((batteryPercentage * BatteryCapacity) / 100) + " W");
  display.println("Percentage: " + String(batteryPercentage) + " %");
  display.println("Kosten: " + GetCurrentPrice() + " ct");

  display.display();
}

void DisplayOff() {
  display.clearDisplay();
  display.display();
}

void SetPixelColorRed() {
  pixels.clear();
  pixels.setBrightness(10);
  pixels.setPixelColor(0, pixels.Color(150, 0, 0));
  pixels.show();
}

void SetPixelColorGreen() {
  pixels.setBrightness(10);
  pixels.setPixelColor(0, pixels.Color(0, 150, 0));
  pixels.show();
}

void SetPixelColorNone() {
  pixels.clear();
  pixels.show();
}


void UpdateDisplay(int line, String data) {
  display.clearDisplay();
  display.setCursor(line * 10, 0);  // Start at top-left corner
  display.print(data);
  display.display();
}

// String PrintTime(time_t epochtime) {
//   return twoDigits(String(epochtime / 3600 % 24)) + ":" + twoDigits(String(epochtime / 60 % 60)) + ":" + twoDigits(String(epochtime % 60));
// }

String PrintTime()
{
  return twoDigits(String(timeinfo.tm_hour)) + ":" + twoDigits(String(timeinfo.tm_min)) + ":" + twoDigits(String(timeinfo.tm_sec));
}

String twoDigits(String input) {
  if (input.length() == 1)
    return ("0" + input);
  else
    return input;
}
