/* 
 *  Frugal IoT example - Data Logger
 *
 * This is a Work in Progress - it works but may not be useful yet.
 * 
 * See https//github.com/frugal-iot/issues/110 for more information.
 * 
 */

#include "frugal_iot.h"

System_Frugal frugal_iot; // Singleton

void setup() {
  frugal_iot.startSerial(); // Encapsulate setting up and starting serial
  // Dont change above here - should be before setup the actuators, controls and sensors

  Control_Gsheets* cg = new Control_Gsheets("gsheets demo", CONTROL_GSHEETS_URL);
  frugal_iot.controls->add(cg);
  cg->track("temperature", frugal_iot.mqtt->path("sht/temperature")); // TODO-141 probably wont work as MQTT not setup

  // Dont change below here - should be after setup the actuators, controls and sensors
  frugal_iot.setup(); 
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop();
}
