File myFile;



marstek_load batteryconfiguration[24];
const char* fileName = "/configuration.csv";

void InitializeLittleFs() {
  // Initialize LittleFS
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
    return;
  }
}

void InitializeSdCard() {
  Serial.println("Setup sd card ");
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card MOUNT FAIL");
  } else {
    Serial.println("SD Card MOUNT SUCCESS");
    if (!readFromFile()) {
      initializeLoad();
      writeToFile();
    }
  }
}




void initializeLoad() {
  // initialize load
  for (int x = 0; x < 24; x++) {
    batteryconfiguration[x].enabled = false;
    batteryconfiguration[x].power = 0;
    batteryconfiguration[x].automatic = 0;
  }
}


int GetBatteryLoadHour(int hour) {
  return batteryconfiguration[hour].power;
}

bool GetAutomaticHour(int hour) {
  return batteryconfiguration[hour].automatic;
}

bool GetEnabledHour(int hour) {
  return batteryconfiguration[hour].enabled;
}

marstek_load GetBatteryControlInfo(int hour)
{
  marstek_load config=batteryconfiguration[hour];
  Serial.print("Uur: ");
  Serial.print(hour);
  Serial.print("Enabled: ");
  Serial.print(config.enabled);
  Serial.print("power: ");
  Serial.print(config.power);
  Serial.print("automatic: ");
  Serial.print(config.automatic);
  

  return config;
}


bool readFromFile() {
  byte i = 0;             //counter
  char inputString[100];  //string to hold read string

  //now read it back and show on Serial monitor
  // Check to see if the file exists:
  if (!SD.exists(fileName)) {
    Serial.print(fileName);
    Serial.println("doesn't exist.");
    return false;
  }
  Serial.print("Reading from:");
  Serial.println(fileName);
  myFile = SD.open(fileName);


  while (myFile.available()) {
    char inputChar = myFile.read();  // Gets one byte from serial buffer
    if (inputChar == '\n')           //end of line (or 10)
    {
      inputString[i] = 0;  //terminate the string correctly
      Serial.println(inputString);
      processline(String(inputString));
      i = 0;
    } else {
      inputString[i] = inputChar;  // Store it
      i++;                         // Increment where to write next
      if (i > sizeof(inputString)) {
        Serial.println("Incoming string longer than array allows");
        Serial.println(sizeof(inputString));
        return false;
      }
    }
  }
  return true;
}

void processline(String data) {
  String result[4];
  splitString(data, ';', result);
  int item = result[0].toInt();
  bool automatic = result[1].toInt();
  int load = result[2].toInt();
  bool enabled = result[3].toInt();
  batteryconfiguration[item].automatic = automatic;
  batteryconfiguration[item].power = load;
  batteryconfiguration[item].enabled = enabled;
}

void splitString(String data, char delimiter, String* result) {
  int index = 0;
  int startIndex = 0;
  int endIndex = data.indexOf(delimiter);

  while (endIndex != -1) {
    result[index++] = data.substring(startIndex, endIndex);
    startIndex = endIndex + 1;
    endIndex = data.indexOf(delimiter, startIndex);
  }
  result[index] = data.substring(startIndex);
}

void updatebatteryconfiguration(int hour, bool automatic, int power, bool enabled) {
  if (hour >= 0 && hour < 24) {
    Serial.println("updating configuration");
    batteryconfiguration[hour].automatic = automatic;
    batteryconfiguration[hour].power = power;
    batteryconfiguration[hour].enabled = enabled;
    writeToFile();
  }
}


void writeToFile() {
  myFile = SD.open(fileName, FILE_WRITE);
  if (myFile)  // it opened OK
  {
    Serial.print("Writing to:");
    Serial.println(fileName);
    for (int x = 0; x < 24; x++) {
      myFile.print(String(x));
      myFile.print(";");
      myFile.print(String(batteryconfiguration[x].automatic));
      myFile.print(";");
      myFile.print(String(batteryconfiguration[x].power));
      myFile.print(";");
      myFile.println(String(batteryconfiguration[x].enabled));
    }
    myFile.close();
    Serial.println("Writing done");
  } else
    Serial.print("Error opening:");
  Serial.print(fileName);
  Serial.println(" for writing");
}

void deleteFile() {
  //delete a file:
  if (SD.exists(fileName)) {
    Serial.println("Removing simple.txt");
    SD.remove(fileName);
    Serial.println("Done");
  }
}
