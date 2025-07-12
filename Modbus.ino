ModbusMaster node;
unsigned long lastModbusCommunication;

bool modbusOkay = false;  // False if Modbus error seen
bool lastModbusInfo;
int lastpower = 0;

// Create a mutex handle
SemaphoreHandle_t xMutex;

RTInfo_t rtdata[] =  // Real-time data from Venus
  {
    // reg, type, desc, factor, waarde
    { 32104, "u16", "SOC", 1, 25, "procent", 1 },                // SOC    - State of charge in procenten, bijv 14
    { 32202, "s32", "AC power bat", 1, 0, "Wat", 1 },            // ACPW   - AC power into the grid, bijv -198
    { 35100, "u16", "Inverter-state", 1, 0, "", 0 },             // INVST  - Inverters state 0..5
    { 42000, "u16", "Control mode", 1, 90, "", 0 },              // CMODE  - modbus control mode
    { 44000, "u16", "Charge cutoff", 10, 90, "procent", 0 },     // CHGCC  - Charging cutoff capacity 80..100%
    { 44001, "u16", "Discharge cutoff", 10, 12, "procent", 0 },  // DISCC  - Discharging cutoff capacity 12..30%
    { 44002, "u16", "max charge power", 1, 100, "Wat", 0 },      // MAXCHG - Max charge power 200..5000W
    { 44003, "u16", "max discharge power", 1, 100, "Wat", 0 },   // MAXDIS - Max discharge power 200..5000W
    { 33000, "u32", "Total charging", 100, 0, "Kwh", 0 },        // TCHARGE - Total charching energy
    { 33002, "u32", "Total discharging", 100, 0, "kwh", 0 },     // TCHARGE - Total charching energy
    { 32102, "s32", "Laadt met", 1, 0, "Wat", 1 }                // CURBC  - Huidig vermogen opladen (negatief is ontladen)


  };

struct controlout_t  // Structure of control output data
{
  uint16_t mb_regnr;  // ModBus registernummer
  char name[30];      // Naam van het register
  int16_t value;      // Output data
};

controlout_t ctdata[] =  // Structure of control output data
  {
    { 42000, "RS485 Control mode", 0x55AA },  // CM485  - Enable/ disable RS485 control mode
    { 42010, "Forcible charge", 0 },          // FORCE  - 0 = stop, 1 = charge, 2 = discharge
    { 42020, "Charge power", 0 },             // COUT   - Forcible charge power
    { 42021, "DisCharge power", 0 },          // DOUT   - Forcible discharge power
  };

#define NUMIREGS (sizeof(rtdata) / sizeof(rtdata[0]))
#define NUMOREGS (sizeof(ctdata) / sizeof(ctdata[0]))
#define DEBUG_BUFFER_SIZE 160



void preTransmission() {
  //digitalWrite ( RS485_pin_de, HIGH ) ;
  delayMicroseconds(10);
  while (Serial1.available())  // Flush all input, normally some null chars
  {
    //int c =
    Serial1.read();
    //dbgprint ( "MPpre flushed 0x%02X", c ) ;
  }
}

void postTransmission() {
  //delayMicroseconds ( 10 ) ;                                // Make sure transmission finished
  //digitalWrite ( RS485_pin_de, LOW ) ;
  delay(2);                    // Make sure R/T is quiet
  while (Serial1.available())  // Flush stray input
  {
    Serial1.read();  // Usually read 0x00 character
  }
}


void setupModbus() {

  pinMode(PIN_5V_EN, OUTPUT);

  UpdateDisplay(1, "Pin_5v_...");

  digitalWrite(PIN_5V_EN, HIGH);
  pinMode(RS485_EN_PIN, OUTPUT);
  pinMode(RS485_SE_PIN, OUTPUT);

  UpdateDisplay(1, "RS485_SE_PIN_...");

  digitalWrite(RS485_SE_PIN, HIGH);


  UpdateDisplay(1, "RRS485_EN_PIN_...");
  digitalWrite(RS485_EN_PIN, HIGH);

  // Initialize the mutex
  xMutex = xSemaphoreCreateMutex();
  if (xMutex == NULL) {
    Serial.println("Mutex creation failed");
  } else {
    Serial.println("Mutex created successfully");
  }


  Serial1.begin(115200, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  delay(100);
  postTransmission();
  node.begin(1, Serial1);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
}


bool get_modbus_data_regs(bool monitorOnly) {
  uint8_t res;       // Result modbus functions
  bool fres = true;  // Function result, assume positive
  uint8_t i;         // Loop control
  uint16_t regnr;    // Register te lezen
  //mb_data_t dt;                              // Type, bijvoorbeeld "s32"
  String dt;
  uint16_t n;        // Aantal register te lezen, 1 = 16 bits woord
  uint16_t value;    // Gelezen waarde
  uint32_t value32;  // Idem, 32 bits
  const char* err;   // Modbus error code


  for (i = 0; i < NUMIREGS; i++)  // Ga de inputs langs
  {
    regnr = rtdata[i].mb_regnr;  // Register te lezen
    if (regnr == 0)              // Is het een Modbus register?
    {
      continue;  // Nee, sla over
    }
    if (monitorOnly && !rtdata[i].monitor) {
      // in geval van monitor only alleen de monitor stappen bijwerken.
      continue;
    }

    dt = rtdata[i].mb_data_type;         // Data type
    n = 1;                               // Meestal 1 woord van 16 bits
    if ((dt == "u32") || (dt == "s32"))  // Dubbel word?
    {
      n = 2;  // Ja, lees 2 woorden van 16 bits
    }
    //dbgprint ( "Lees %d registers vanaf %d(%s)",
    //           n, regnr, rtdata[i].name ) ;

    res = node.readHoldingRegisters(regnr, n);      // Read holdingregister (0x03) from Venus
    if ((modbusOkay = (res == node.ku8MBSuccess)))  // Success?
    {
      value = node.getResponseBuffer(0);  // Ja, leest eerste woord
      if (dt == "u16") {
        value32 = value;  // Converteer u16 naar float
      }
      if (dt == "s16") {
        value32 = (int16_t)value;  // Converteer s16 naar float
      }
      if (dt == "s32") {
        value32 = value << 16 | node.getResponseBuffer(1);
      }
      if (dt == "u32") {
        value32 = value << 16 | node.getResponseBuffer(1);
      }

      delay(20);  // Laat anderen toe

      rtdata[i].value = value32 / rtdata[i].A;  // Scale and store data
      //dbgprint ( "Register inhoud is %d", rtdata[i].value ) ;

    } else {
      Serial.print("MODBUS Error");
      err = get_modbus_errstr(res);     // ModBus fout, haal de errortekst op
      dbgprint("MB: Fout 0x%02X (%s) "  // Modbus I/O error
               "bij lezen van register %d!",
               res, err, regnr);
      fres = false;  // Set negatief function result
      break;         // Doorgaan is zinloos
    }
  }
  return fres;  // Return result as a boolean
}

bool put_modbus_data_regs() {
  uint8_t res;       // Result modbus functions
  bool fres = true;  // Function result, assume positive
  uint8_t i;         // Loop control
  uint16_t regnr;    // Register te lezen
  uint16_t value;    // Gelezen waarde
  const char* err;   // Modbus error code


  for (i = 0; i < NUMOREGS; i++)  // Ga de outputs langs
  {
    regnr = ctdata[i].mb_regnr;  // Register te lezen

    value = ctdata[i].value;  // Waarde te schrijven


    res = node.writeSingleRegister(regnr, value);  // Schrijf holdingregister

    modbusOkay = (res == node.ku8MBSuccess);  // Success of niet
    err = get_modbus_errstr(res);             // Haal errortekst op


    if (!modbusOkay)  // Success?
    {
      dbgprint("MB: Fout 0x%02X (%s) bij schrijven register!",  // Modbus I/O error
               res, err);
      fres = false;  // Set negatief function result
      break;         // Doorgaan is zinloos
    }
  }
  return fres;  // Return result as a boolean
}


// void logModbusData() {
//   int i;
//   for (i = 0; i < NUMIREGS; i++) {
//     Serial.print(rtdata[i].mb_regnr);
//     Serial.print(" waarde: ");
//     Serial.println(rtdata[i].value, HEX);
//   }
// }

void dbgprint(const char* format, ...) {
  static char sbuf[DEBUG_BUFFER_SIZE];  // For debug lines
  va_list varArgs;                      // For variable number of params
  String dbgline;                       // Resulting line

  va_start(varArgs, format);                       // Prepare parameters
  vsnprintf(sbuf, sizeof(sbuf), format, varArgs);  // Format the message
  va_end(varArgs);                                 // End of using parameters

  dbgline = PrintTime() + " :" + String(sbuf);  // Time and info to a String
  Serial.println(dbgline.c_str());              // and the info
}

const char* get_modbus_errstr(uint8_t err) {
  const char* resstr = "?";  // Assume unknowk code

  if (err == node.ku8MBInvalidSlaveID)  // Invalid slave ID?
  {
    resstr = "InvalidSlaveID";
  } else if (err == node.ku8MBResponseTimedOut)  // Response time-out?
  {
    resstr = "ResponseTimedOut";
  } else if (err == node.ku8MBInvalidCRC) {
    resstr = "InvalidCRC";
  } else if (err == node.ku8MBIllegalDataValue) {
    resstr = "IllegalDataValue";
  }
  return resstr;  // Return resulting string
}


String GetBatteryInfoForPage(String registerNummer) {
  String result = "not found";
  int i;
  int registerid = registerNummer.toInt();

  // get all data;
  RTInfo_t data = GetRegisterData(registerid, false);

  result = String(data.value) + " " + data.unity;

  return result;
}


RTInfo_t GetRegisterData(int registerNummer, bool monitorOnly) {
  RTInfo_t result;
  int i;
  if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
    if (lastModbusCommunication < now || monitorOnly != lastModbusInfo) {
      lastModbusInfo = monitorOnly;
      get_modbus_data_regs(monitorOnly);
      lastModbusCommunication = now + 5;
    }
    for (i = 0; i < NUMIREGS; i++) {

      if (rtdata[i].mb_regnr == registerNummer) {
        result = rtdata[i];
        break;
      }
    }
    xSemaphoreGive(xMutex);
  }
  return result;
}


void SetBatteryOutput(int32_t power, int32_t batteryPercentage) {
  int maxBatteryPercentage = GetMaxBatteryPercentage();

  if (power < 0 && batteryPercentage > minBatteryPercentage) {
    if (lastpower != power) {
      ConfigureDisChargePower(power);
    }
  } else if (power > 0 && batteryPercentage < maxBatteryPercentage) {
    if (lastpower != power) {
      ConfigureChargePower(power);
    }
  } else {
    if (power != 0) {
      ConfigureNoPower();
      power = 0;
    }
  }
  if (power != lastpower) {
    if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
      put_modbus_data_regs();
      xSemaphoreGive(xMutex);

      lastpower = power;
    }
  }
}

void ConfigureDisChargePower(int power) {
  int i;
  if (abs(power) > abs(MaxReturnPower))
    power = MaxReturnPower;

  if (abs(power) < abs(minPowerLoad))
    power = minPowerLoad;

  for (i = 0; i < NUMOREGS; i++) {
    if (ctdata[i].mb_regnr == 42010) {
      ctdata[i].value = 2;
    }
    if (ctdata[i].mb_regnr == 42020) {
      ctdata[i].value = 0;
    }
    if (ctdata[i].mb_regnr == 42021) {
      ctdata[i].value = abs(power);
    }
  }
  SetPixelColorGreen();
}


void ConfigureChargePower(int power) {
  int i;
  if (abs(power) > MaxLoadPower)
    power = MaxLoadPower;

  for (i = 0; i < NUMOREGS; i++) {
    if (ctdata[i].mb_regnr == 42010) {
      ctdata[i].value = 1;
    }
    if (ctdata[i].mb_regnr == 42020) {
      ctdata[i].value = abs(power);
    }
    if (ctdata[i].mb_regnr == 42021) {
      ctdata[i].value = 0;
    }
  }
  SetPixelColorRed();
}

void ConfigureNoPower() {
  int i;
  for (i = 0; i < NUMOREGS; i++) {
    if (ctdata[i].mb_regnr == 42010) {
      ctdata[i].value = 0;
    }
    if (ctdata[i].mb_regnr == 42020) {
      ctdata[i].value = 0;
    }
    if (ctdata[i].mb_regnr == 42021) {
      ctdata[i].value = 0;
    }
  }
  SetPixelColorNone();
}

int32_t GetMaxBatteryPercentage() {
  int daynr = weekDay();
  // monday and friday 100% other days just 98%
  if (daynr == 1 || daynr == 5)
    return 100;
  else
    return 97;
}

int GetCurrentBatteyPower() {
  return lastpower;
}