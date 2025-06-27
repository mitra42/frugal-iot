/* 
 *  Frugal IoT example - Sonoff - R2 switch
 * Optional: 
 */

#include "frugal_iot.h"

System_Frugal frugal_iot; // Singleton

void setup() {
  frugal_iot.startSerial(); // Encapsulate setting up and starting serial
  // Relay on Sonoff is on pin 12
  frugal_iot.actuators->add(new Actuator_Digital("relay", "Relay", 12, "purple"));
  ControlHysterisis* cb = new ControlHysterisis("control", "Control", 50, 1, 0, 100);
  frugal_iot.controls->add(cb);
  cb->outputs[0]->wireTo(frugal_iot.mqtt->path("relay/on"));

  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and sysetm
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop();
}

