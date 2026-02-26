#include <SPI.h>
#include <Wire.h>

#include <WiFi.h>

// server configuration
#include <ESPAsyncWebServer.h>


// client configuration from esp32 sources
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
// json library for communication
#include <ArduinoJson.h>

// display and led configuration
#include <Adafruit_NeoPixel.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// time lib
#include <Time.h>

// client configuration
#include <configuration.h>
#include <Secrets.h>

// flash reading reading
#include "LittleFS.h"
#include "SD.h"

// modbus libraries
#include <HardwareSerial.h>
#include <ModbusMaster.h>


unsigned long currentTime;
unsigned long lastPowerUpdate;
unsigned long lastPriceUpdate;
unsigned long lastBatteryChange;
unsigned long lastThingSpeakUpdate;

// battery data
int32_t selectedPower;
int32_t batteryPercentage;
int32_t currentBatteryPower;

// time info
struct tm timeinfo;
time_t now;

WiFiClient client;

// lcd screen
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// webserver
AsyncWebServer server(80);

// Variables for integration
int power = 0;
int returnpower = 0;
int currentLoad = 0;
float arr_m[24];

float profit = 0;
int lastday = 0;
bool allowExternalPower=false;


void setup() {
  Serial.begin(9600);
  while (!Serial)
    ;

  delay(1000);
  initializeThingSpeak();

  Initialize_Display();


  delay(1000);
  Initialize_Wifi_And_Server();
  delay(500);
  UpdateDisplay(1, "Configure time...");
  initTime(MY_TIME_ZONE);
  UpdateDisplay(1, "Time configured...");
  // initialize the sd card
  UpdateDisplay(1, "Initialize fs...");
  InitializeLittleFs();
  UpdateDisplay(1, "Initialize SD...");
  InitializeSdCard();
  delay(1000);
  UpdateDisplay(1, "Initialize Modbus...");
  setupModbus();
}

void loop() {
  bool wifiConnected = false;
  FillLocalTime();

  ResetProfitIfNeeded();

  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  } else {
    wifiConnected = true;

    currentTime = now;
    if (lastPriceUpdate < currentTime) {
      lastPriceUpdate = currentTime + (60 * 60);
      GetTibberData();
    }


    delay(500);
    if (lastPowerUpdate < currentTime) {
      lastPowerUpdate = currentTime + 10;  // 10 secs
      Serial.println("beeclear updated: " + PrintTime());
      GetCurrentLoad();
    }
    // update the info for the display
    UpdateBatteryInfo();
    // use the updated data if needed
    if (lastBatteryChange < currentTime) {
      lastBatteryChange = currentTime + 20;
      ControlBattery(wifiConnected);
    }
  }
  // update thingspeak every minut
  if (lastThingSpeakUpdate < currentTime) {
    RTInfo_t batpercentage = GetRegisterData(32104, true);
    float price = (float)GetCurrentPriceValue();

    SendThingSpeakData(GetCurrentBatteyPower(), batpercentage.value, price);
    lastThingSpeakUpdate = currentTime + 60;
  }


  DisplayInfo();
  delay(500);
}

void UpdateBatteryInfo() {
  RTInfo_t batinfo;
  batinfo = GetRegisterData(32202, true);
  currentBatteryPower = batinfo.value;
  // get percentage
  batinfo = GetRegisterData(32104, true);
  batteryPercentage = batinfo.value;
}

void ControlBattery(bool wifiConnected) {

  marstek_load scheduleConfiguration;
  // get the configuration of the battery
  scheduleConfiguration = GetBatteryControlInfo(currentHour());

  if (scheduleConfiguration.enabled) {
    allowExternalPower=false;
    if (scheduleConfiguration.automatic) {
      if (wifiConnected) {
        ControlBatteryAutomatic(currentBatteryPower);
      }
    } else {
      ControlBatteryManuel(scheduleConfiguration.power, currentBatteryPower);
    }
  } else {
    if(allowExternalPower==false)
    {
    CalculateBatteryPower(currentBatteryPower);
    SetBatteryOutput(0, batteryPercentage);
    Serial.println("No Power configured");
    }
  }
}

void ControlBatteryManuel(int32_t requestedPower, int32_t currentBatteryPower) {

  int32_t difference = requestedPower - currentBatteryPower;
  Serial.print("manual mode: ");
  Serial.println(difference);
  if (abs(difference) > 20) {
    SetBatteryOutput(requestedPower, batteryPercentage);
  }
}

bool ExternalControlBattery(int32_t requestedPower)
{
  marstek_load scheduleConfiguration;
  // get the configuration of the battery
  scheduleConfiguration = GetBatteryControlInfo(currentHour());

  if(scheduleConfiguration.enabled)
  {
    Serial.println("External control not possible");
    return false;
  }
  allowExternalPower=true;
  ControlBatteryManuel(requestedPower,currentBatteryPower);
  return true;
}

void ControlBatteryAutomatic(int32_t currentBatteryPower) {
  int32_t requiredPower = 0;
  // invert the amount
  currentBatteryPower = currentBatteryPower * -1;

  Serial.println("auto mode: ");

  requiredPower = CalculateBatteryPower(currentBatteryPower);
  SetBatteryOutput(requiredPower, batteryPercentage);
}

int CalculateBatteryPower(int currentBatteryPower) {
  int requiredPower = 0;
  int level = GetPriceLevel();
  // calculate the load

  if (currentLoad < 0 && currentBatteryPower <= 0) {
    requiredPower = abs(currentLoad) + currentBatteryPower;
  } else if (currentLoad > 0 && currentBatteryPower <= 0) {
    requiredPower = -(currentLoad + abs(currentBatteryPower));
  } else if (currentLoad > 0 && currentBatteryPower >= 0) {
    requiredPower = (currentBatteryPower - currentLoad);
  } else if (currentLoad < 0 && currentBatteryPower >= 0) {
    requiredPower = abs(currentLoad) + currentBatteryPower;
  } else {
    requiredPower = selectedPower;
  }

  // als uitgerekende verschil < 20 dan laten we alles bij het oude.
  if (abs(currentBatteryPower - requiredPower) < 20) {
    requiredPower = currentBatteryPower;
  }

  // minumum power export
  if (requiredPower < 0 and abs(requiredPower) < abs(minPowerexport))
    requiredPower = minPowerexport;

  // no power below 50 watt load
  else if (requiredPower > 0 and requiredPower < minPowerload)
    requiredPower = 0;

  // cheap hour
  if (requiredPower < 0 && requiredPower > (MaxReturnPower / 2) && level < 3) {
    Serial.println("no export price to low ");
    requiredPower = 0;
  }

  // no load if pricelevel is to big
  else if (requiredPower > 0 && level > 3) {
    Serial.println("no load price to high ");
    requiredPower = 0;
  }

if(currentHour() >= 11 && currentHour() < 16)
{
  // tot 400 watt we do not want to exost the battery proces are low
  if(requiredPower >-400 && requiredPower < 0)
  {
    requiredPower=0;
    Serial.println("no load lower then 400 watt between 11 & 16");
  }
}


  // set the max amounts
  if (requiredPower < MaxReturnPower)
    requiredPower = MaxReturnPower;
  else if (requiredPower > MaxLoadPower)
    requiredPower = MaxLoadPower;

  Serial.print("CurrentLoad: ");
  Serial.print(currentLoad);
  Serial.print("currentBatteryPower: ");
  Serial.print(currentBatteryPower);
  Serial.print("requiredPower: ");
  Serial.print(requiredPower);

  selectedPower = requiredPower;
  return requiredPower;
}
