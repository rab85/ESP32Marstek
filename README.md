# ESP32Marstek
Marstek modbus koppeling met een esp32.
De modbus koppeling is gebasseerd op de EdZelf marstek koppeling.
Hierbij is de NOM functie wel aangepast.

# Uitgangspunten
P1 meter is een beeclear device. Deze wordt later vervangen door de pulse info vanuit tibber
Tibber wordt gebruikt voor de price info. (Deze info wordt alleen weergegeven).
Display is een Adafruit 320*120 display.
Board is een lily go tcan 485 (esp 32 controller).
Elke 20 seconden wordt de battery bijgestuurd. Wanneer het absolute verschil meer dan 20 wat is wordt er bijgestuurd.

# Web interface
Middels de webpagina op de marstekcontroller kun je een schema instellen en de details zien.

# te configureren
In configuration.h moet je de wifi instellingen configureren
In tibber_configuration.h moet je de tibber key plaatsen.
In configuration.h kun je ook de battery uitgangspunten configuren:
const int32_t minBatteryPercentage=15;
const int32_t maxBatteryPercentage=98;

const int MaxReturnPower = -800;
const int MaxLoadPower = 1000;
const int BatteryCapacity = 2500;



