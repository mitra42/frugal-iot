/* 
 *  Frugal IoT example - ENS160 AHT21 environment sensor
 * 
 */

#include "frugal_iot.h"

System_Frugal frugal_iot; // Singleton

void setup() {
  frugal_iot.startSerial(); // Encapsulate setting up and starting serial
  frugal_iot.sensors->add(new Sensor_ensaht("ensaht","ENS160 AHT21"));
  // system_oled and actuator_ledbuiltin added automatically on boards that have them.
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and sysetm
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop();
}

