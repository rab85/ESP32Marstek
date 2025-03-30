// wifi settings

const char* ssid = "SID";      // CHANGE IT
const char* password = "PASSWORD";  // CHANGE IT

// time settings
#define MY_NTP_SERVER "nl.pool.ntp.org"
// amsterdam time zone
#define MY_TIME_ZONE "CET-1CEST,M3.5.0,M10.5.0/3"


struct marstek_load {
  bool automatic;
  int power;
  bool enabled;
};

struct RTInfo_t  // Structure of Real-time input data
{
  uint16_t mb_regnr;    // ModBus registernummer
  String mb_data_type;  // Soort register
  char name[30];        // Naam van het register
  int32_t A;            // Ruwe naar fysieke waarde omrekenfactor, deelfactor
  int32_t value;        // Measured data in fysieke grootheid
  String unity;         // eenheid
  bool monitor;
};


const int32_t minBatteryPercentage = 15;
const int32_t maxBatteryPercentage = 98;

const int MaxReturnPower = -800;
const int MaxLoadPower = 1000;
const int BatteryCapacity = 2500;

// display info
#define SCREEN_WIDTH 128     // OLED display width, in pixels
#define SCREEN_HEIGHT 64     // OLED display height, in pixels
#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define ROTATION 0           // Rotates text on OLED 1=90 degrees, 2=180 degrees
#define SDA_PIN 32
#define SCL_PIN 33


// info rgb led
#define PIN 4
#define NUMPIXELS 1
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
#define DELAYVAL 500

// Define SD card connection
#define SD_MOSI 15
#define SD_MISO 02
#define SD_SCLK 14
#define SD_CS 13

// modbus configuratie
#ifndef __CONFIG_H__
#define __CONFIG_H__

// PIN
#define PIN_5V_EN 16

#define CAN_TX_PIN 26
#define CAN_RX_PIN 27
#define CAN_SE_PIN 23

#define RS485_EN_PIN 17  // 17 /RE
#define RS485_TX_PIN 22  // 21
#define RS485_RX_PIN 21  // 22
#define RS485_SE_PIN 19  // 22 /SHDN

#define SD_MISO_PIN 2
#define SD_MOSI_PIN 15
#define SD_SCLK_PIN 14
#define SD_CS_PIN 13

#define WS2812_PIN 4

#endif
