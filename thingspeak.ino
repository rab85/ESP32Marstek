#include "ThingSpeak.h"

void initializeThingSpeak() {
  ThingSpeak.begin(client);  // Initialize ThingSpeak
}


void SendThingSpeakData(int power, int batteryPercentage, float price) {
  ThingSpeak.setField(1, power);
  ThingSpeak.setField(2, batteryPercentage);
  ThingSpeak.setField(3, price);

  int x = ThingSpeak.writeFields(thingspeakChannelNumber, thingspeakWriteAPIKey);
  if (x == 200) {
    Serial.println("Channel update successful.");
  } else {
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }
}