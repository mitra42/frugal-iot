/* 
 *  Frugal IoT example - LilyGo HiGrow temperature and humidity sensor
 * 
 */

#include "frugal_iot.h"

// Change the parameters here to match your ... 
// organization, project, id, description
System_Frugal frugal_iot("dev", "developers", "lilygohigrow", "LilyGo HiGrow plant sensor");

void setup() {
  frugal_iot.startSerial(); // Encapsulate setting up and starting serial
  frugal_iot.fs_LittleFS->pre_setup();
  // Override MQTT host, username and password if you have an "organization" other than "dev" (developers)
  frugal_iot.configure_mqtt("frugaliot.naturalinnovation.org", "dev", "public");

  // Configure power handling - type, cycle_ms, wake_ms 
  // power will be awake wake_ms then for the rest of cycle_ms be in a mode defined by type 
  // Loop= awake all the time; 
  // Light = Light Sleep; 
  // LightWiFi=Light + WiFi on (not working); 
  // Modem=Modem sleep - works but negligable power saving
  // Deep - works but slow recovery and slow response to UX so do not use except for multi minute cycles. 
  frugal_iot.configure_power(Power_Loop, 30000, 30000); // Take a reading every 30 seconds - awake all the time

  // system_oled and actuator_ledbuiltin added automatically on boards that have them.

  // Add local wifis here, or see instructions in the wiki for adding via the /data
  frugal_iot.wifi->addWiFi(F("mywifissid"),F("mywifipassword"));
  
  frugal_iot.sensors->add(new Sensor_DHT("DHT", GPIO_NUM_16, true));
  // Soil sensor 0%=4095 100%=0 pin=32 smooth=0 color=brown
  frugal_iot.sensors->add(new Sensor_Soil("soil", "Soil", 32, 4095, -100/4095, "brown", true));
  // The salt sensor does not seem to work - got incorrect readings. TODO debug
  // Salt sensor 0%=0 100%=5000 pin=34 color=green
  // frugal_iot.sensors->add(new Sensor_Analog("salt", "Salt", 34, 1, 0, 100, 0, 0.02, "green", true));
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

