/* 
 *  Frugal IoT example - Data Logger
 *
 * This is a Work in Progress - it works but may not be useful yet.
 * 
 * See https//github.com/frugal-iot/issues/110 for more information.
 * 
 */

#include "frugal_iot.h"

// Change the parameters here to match your ... 
// organization, project, id, description
System_Frugal frugal_iot("dev", "developers", "blinken", "Blinking light demo");

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
  
  // Dont change above here - should be before setup the actuators, controls and sensors

  Control* cb = new ControlBlinken("blinken", "Blinken", 5, 2);
  frugal_iot.controls->add(cb);
  cb->outputs[0]->wireTo(frugal_iot.mqtt->path("ledbuiltin/id")); //TODO-141 probably wont work as mqtt not setup yet

  // Dont change below here - should be after setup the actuators, controls and sensors
  frugal_iot.setup(); 
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop();
}

