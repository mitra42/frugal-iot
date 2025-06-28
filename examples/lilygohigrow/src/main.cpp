/* 
 *  Frugal IoT example - LilyGo HiGrow temperature and humidity sensor
 * 
 */

#include "frugal_iot.h"

System_Frugal frugal_iot; // Singleton

void setup() {
  frugal_iot.startSerial(); // Encapsulate setting up and starting serial
  frugal_iot.sensors->add(new Sensor_DHT("DHT", GPIO_NUM_16, true));
  // Soil sensor 0%=4095 100%=0 pin=32 smooth=0 color=brown
  frugal_iot.sensors->add(new Sensor_Soil("soil", "Soil", 4095, 0, 32, 0, "brown", true));
  // The salt sensor does not seem to work - got incorrect readings. TODO debug
  // Salt sensor 0%=0 100%=5000 pin=34 smooth=4 color=green
  // frugal_iot.sensors->add(new Sensor_Analog("salt", "Salt", 34, 4, 0, 5000, "green", true));
  // actuator_ledbuiltin automatically added, but doesnt seem to work on LilyGo HiGrow

  // Battery sensor on pin 33 - but note battery didnt arrive so not tested TODO test
  frugal_iot.sensors->add(new Sensor_Battery(33, 6.6F));  // TODO-57 will rarely be as simple as this

  // Light sensor BH1750 TODO see notes in sensor_bh1750.cpp,h about I2C pin conflicts
  frugal_iot.sensors->add(new Sensor_BH1750("lux", "Lux", SENSOR_BH1750_ADDRESS, true));

  // TODO-115 there is a relay pin - haven't tested it yet - not sure which pin it is on
  // frugal_iot.actuators->add(new Actuator_Digital("relay", "Relay", XX, "purple"));

  // Button on pin 35 - not sure if this is tested yet
  frugal_iot.sensors->add(new Sensor_Button("button", "Button", 35, "purple"));

  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and sysetm
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop();
}

