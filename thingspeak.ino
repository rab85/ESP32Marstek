#include "ThingSpeak.h"

void initializeThingSpeak() {
  ThingSpeak.begin(client);  // Initialize ThingSpeak
}


void SendThingSpeakData(int power, int batteryPercentage, float price) {
  // calculate the profit
  profit += Calculateprofit(power, price);

  ThingSpeak.setField(1, power);
  ThingSpeak.setField(2, batteryPercentage);
  ThingSpeak.setField(3, price);
  ThingSpeak.setField(4, profit);

  int x = ThingSpeak.writeFields(thingspeakChannelNumber, thingspeakWriteAPIKey);
  if (x == 200) {
    Serial.println("Channel update successful.");
  } else {
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }
}
void ResetProfitIfNeeded() {
  // reset profit and lost
  if (lastday != currentDay()) {
    lastday = currentDay();
    profit = 0;
  }
}

float Calculateprofit(int power, float price) {
  return ((float)1 / 60) * ((float)power / 1000) * (price/100);
}
