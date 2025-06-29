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

