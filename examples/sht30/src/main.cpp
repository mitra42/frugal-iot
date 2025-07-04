/* 
 *  Frugal IoT example - SHT30 temperature and humidity sensor
 * 
 * Optional: SENSOR_SHT_ADDRESS - defaults to 0x44, (note the D1 shields default to 0x45)
 */

// defines SENSOR_SHT_ADDRESS if dont define here or in platformio.ini
#include "frugal_iot.h"
// Change the parameters here to match your ... 
// organization, project, id, description
System_Frugal frugal_iot("dev", "developers", "sht30", "SHT30 Temperature and Humidity Sensor"); 

void setup() {
  frugal_iot.startSerial(); // Encapsulate setting up and starting serial

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
  //frugal_iot.wifi->addWiFi(F("Silver"),F("O2IL-w8k2-w6ki-TzqG"));
  frugal_iot.wifi->addWiFi(F("EXETEL 8BFC34 2.4G"),F("urGGFHQN"));
  
  // Add sensors, actuators and controls
  frugal_iot.sensors->add(new Sensor_SHT("SHT", SENSOR_SHT_ADDRESS, &Wire, true));
  
  // Dont change below here - should be after setup the actuators, controls and sensors
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and sysetm
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop();
}

