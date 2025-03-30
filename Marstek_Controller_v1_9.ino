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

// battery data
int32_t selectedPower;
int32_t batteryPercentage;
int32_t currentBatteryPower;

// time info
struct tm timeinfo;
time_t now;


// lcd screen
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// webserver
AsyncWebServer server(80);

// Variables for integration
int power = 0;
int returnpower = 0;
int currentpower = 0;
float arr_m[24];


void setup() {
  Serial.begin(9600);
  while (!Serial)
    ;

  delay(1000);

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
  delay(1000);
  InitializeSdCard();
  delay(1000);
  setupModbus();
}

void loop() {
  bool wifiConnected = false;
  FillLocalTime();

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

    if (lastBatteryChange < currentTime) {
      lastBatteryChange = currentTime + 20;
      ControlBattery(wifiConnected);
    }
  }

  DisplayInfo();
  delay(500);
}

void ControlBattery(bool wifiConnected) {
  RTInfo_t batinfo;
  marstek_load scheduleConfiguration;

  // get the configuration of the battery
  scheduleConfiguration = GetBatteryControlInfo(currentHour());

  if (scheduleConfiguration.enabled) {
    // get current AC power
    batinfo = GetRegisterData(32202, true);
    currentBatteryPower = batinfo.value;
    // get percentage
    batinfo = GetRegisterData(32104, true);
    batteryPercentage = batinfo.value;

    if (scheduleConfiguration.automatic) {
      if (wifiConnected) {
        ControlBatteryAutomatic(currentBatteryPower);
      }
    } else {
      ControlBatteryManuel(scheduleConfiguration.power, currentBatteryPower);
    }
  } else {
    CalculateBatteryPower(currentBatteryPower);
    SetBatteryOutput(0, batteryPercentage);
    Serial.println("No Power configured");
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


void ControlBatteryAutomatic(int32_t currentBatteryPower) {
  int32_t requiredPower = 0;
  // invert the amount
  currentBatteryPower = currentBatteryPower * -1;
  bool noPower = false;

  Serial.print("auto mode: ");

  requiredPower = CalculateBatteryPower(currentBatteryPower);

  if (currentBatteryPower == 0 && requiredPower > -50 && requiredPower < 200) {
    SetBatteryOutput(0, batteryPercentage);
    noPower = true;
  } else {
    SetBatteryOutput(requiredPower, batteryPercentage);
  }
  Serial.print(" NoPower: ");
  Serial.println(noPower);
}

int CalculateBatteryPower(int currentBatteryPower) {
  int requiredPower = 0;

  if (abs(currentpower) < 20) {
    requiredPower = selectedPower;
  } else {

    // calculate the load

    if (currentpower < 0 && currentBatteryPower <= 0) {
      requiredPower = abs(currentpower) + currentBatteryPower;
    } else if (currentpower > 0 && currentBatteryPower <= 0) {
      requiredPower = -(currentpower + abs(currentBatteryPower));
    } else if (currentpower > 0 && currentBatteryPower >= 0) {
      requiredPower = -(currentpower - currentBatteryPower);
    } else if (currentpower < 0 && currentBatteryPower >= 0) {
      requiredPower = abs(currentpower) + currentBatteryPower;
    } else {
      requiredPower = selectedPower;
    }
  }
  // set the max amounts
  if (requiredPower < MaxReturnPower)
    requiredPower = MaxReturnPower;
  if (requiredPower > MaxLoadPower)
    requiredPower = MaxLoadPower;

  Serial.print("CurrentPower: ");
  Serial.print(currentpower);
  Serial.print(" currentBatteryPower: ");
  Serial.print(currentBatteryPower);
  Serial.print(" requiredPower: ");
  Serial.print(requiredPower);

  selectedPower = requiredPower;
  return requiredPower;
}
