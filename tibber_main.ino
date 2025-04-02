#include <tibber_configuration.h>

// A single price valid for one hour
struct tibber_price {
  //First 14 characters are sufficient, other characters are zero
  char starttime[14];
  int day;
  int hour;
  int price;       // price as 0.1 cent
  int level;       // Price Level (1-5) as provided by Tibber (Very Cheap, Cheap, Normal, Expensive, Very Expensive)
  boolean isNull;  // True, if data not available
};

//Prices for the next 24 hours
struct tibber_prices {
  tibber_price price[24];
  int minimumPrice = 1000;  //Preis als int in zehntel-Cent
  int maximumPrice = 1;     //Preis als int in zehntel-Cent
};

// Prices as global variable
tibber_prices PRICES;

void GetTibberData() {
  

  NetworkClientSecure *client = new NetworkClientSecure;
  if (client) {
    client->setCACert(root_ca);
    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before NetworkClientSecure *client is
      HTTPClient https;
      Serial.print("[HTTPS] begin...\n");
      if (https.begin(*client, tibberApi)) {  // HTTPS
        Serial.print("[HTTPS] GET...\n");
        // start connection and send HTTP header


        https.addHeader("Content-Type", "application/json");
        // Personal API key to get the data
        https.addHeader("Authorization", "Bearer " + tibber_accces_token);
        //Query for the tibber API
        String payload = "{\"query\": \"{ viewer { homes { currentSubscription{ priceInfo{ current{ total  startsAt level} today { total startsAt level} tomorrow { total startsAt level} }}}} }\" }";

        int httpCode = https.POST(payload);
        if (httpCode == HTTP_CODE_OK) {

          DynamicJsonDocument jsonDoc(8000);

          DeserializationError error = deserializeJson(jsonDoc, https.getStream());

          if (error) {
            Serial.println(error.c_str());
            Serial.println("Error parsing JSON response");
          }
          Serial.println("Data collected");
          parseTibberJson(jsonDoc);

          https.end();
        } else {
          Serial.printf("[HTTPS] Unable to connect\n");


          // End extra scoping block
        }

        delete client;
      } else {
        Serial.println("Unable to create client");
      }
    }
  }
}


//Parse the JSON Document and fill the tibber_prices variable
void parseTibberJson(DynamicJsonDocument jsonDoc) {
  //temporay price for a single hour
  tibber_price tmpPrice;
  int size = sizeof(PRICES.price);
  // 0 is today, 1 is tomorrow
  int day = 0;
  int hour = currentHour()-1;
  int tmpHour;
  // We need the first six letters to parse the price level
  char tmpLevel[6];
  // Reset maximum/minimum price for the next 24 hours - Values will be overwritten with first price
  PRICES.maximumPrice = 1;
  PRICES.minimumPrice = 1000;
  //Loop over 24 hours
  for (int i = 0; i <= 23; i++) {
    //Calculate the hour for the loop
    tmpHour = i + hour;
    if (tmpHour > 23) {
      tmpHour = tmpHour - 24;
      day = 1;
    }
    // Check if we have a price for this hour
    if (jsonDoc["data"]["viewer"]["homes"][0]["currentSubscription"]["priceInfo"][(day == 0) ? "today" : "tomorrow"].isNull() || jsonDoc["data"]["viewer"]["homes"][0]["currentSubscription"]["priceInfo"][(day == 0) ? "today" : "tomorrow"][tmpHour].isNull() || jsonDoc["data"]["viewer"]["homes"][0]["currentSubscription"]["priceInfo"][(day == 0) ? "today" : "tomorrow"][tmpHour]["total"].isNull()) {
      tmpPrice.isNull = true;
      PRICES.price[i] = tmpPrice;
      Serial.print("Wert ist Null: ");
      Serial.println(i);
    } else {
      //Fill the price in the tmp variable
      tmpPrice.price = (int)(1000 * (double)jsonDoc["data"]["viewer"]["homes"][0]["currentSubscription"]["priceInfo"][(day == 0) ? "today" : "tomorrow"][tmpHour]["total"]);
      if (PRICES.maximumPrice < tmpPrice.price) {
        PRICES.maximumPrice = tmpPrice.price;
      }
      if (PRICES.minimumPrice > tmpPrice.price) {
        PRICES.minimumPrice = tmpPrice.price;
      }
      strlcpy(tmpPrice.starttime,
              jsonDoc["data"]["viewer"]["homes"][0]["currentSubscription"]["priceInfo"][(day == 0) ? "today" : "tomorrow"][tmpHour]["startsAt"],
              sizeof(tmpPrice.starttime));
      if (tmpPrice.starttime != NULL) {
        tmpPrice.day = GetIntValue(tmpPrice.starttime, 8, 2);
        tmpPrice.hour = GetIntValue(tmpPrice.starttime, 11, 2);
      }
      strlcpy(tmpLevel,
              jsonDoc["data"]["viewer"]["homes"][0]["currentSubscription"]["priceInfo"][(day == 0) ? "today" : "tomorrow"][tmpHour]["level"],
              sizeof(tmpLevel));
      // Store the level as an int
      switch (tmpLevel[0]) {
        case 'N':
          tmpPrice.level = 3;
          break;
        case 'E':
          tmpPrice.level = 4;
          break;
        case 'C':
          tmpPrice.level = 2;
          break;
        case 'V':
          tmpPrice.level = (tmpLevel[5] = 'E') ? 5 : 1;
          break;
      }
      tmpPrice.isNull = false;
      //Store the price in the global structure
      PRICES.price[i] = tmpPrice;
    }
  }
  printPrices();
}

//Print the prices over Serial
void printPrices() {
  for (int i = 0; i <= 23; i++) {
    if (!PRICES.price[i].isNull) {
      Serial.printf("Beginn: %s Level %i Price: %i \n", PRICES.price[i].starttime, PRICES.price[i].level, PRICES.price[i].price);
    }
  }
  Serial.print("Minimum price: ");
  Serial.println(PRICES.minimumPrice);
  Serial.print("Maximum Price: ");
  Serial.println(PRICES.maximumPrice);
}

String GetCurrentPrice() {
  int day = currentDay();
  int hour = currentHour();
  for (int i = 0; i <= 23; i++) {
    if (!PRICES.price[i].isNull) {
      if (PRICES.price[i].day == day && PRICES.price[i].hour == hour)
      //if (PRICES.price[i].hour == hour)
        return String((double)PRICES.price[i].price / 10);
    }
  }
  return "onbekend";
}

String GetPriceForHour(int hour) {
   for (int i = 0; i <= 23; i++) {
    if (!PRICES.price[i].isNull) {
      if (PRICES.price[i].hour == hour)
        return String((double)PRICES.price[i].price / 10);
    }
  }
  return "onbekend";
}


int GetIntValue(char time[14], int startpos, int length) {
  String Data = String(time);
  Serial.print("Convertedtime:");
  Serial.println(Data);
  String selection = Data.substring(startpos, startpos + length);
  Serial.println(selection);
  return selection.toInt();
}
