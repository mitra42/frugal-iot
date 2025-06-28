/* 
 *  Frugal IoT example - MS5803 pressure sensor
 * 
 * Note that code appears to work, but testing gets nonsense readings - maybe a faulty device
 * 
 */

// defines SENSOR_SHT_ADDRESS if dont define here or in platformio.ini
#include "frugal_iot.h"

System_Frugal frugal_iot; // Singleton

void setup() {
  frugal_iot.startSerial(); // Encapsulate setting up and starting serial
  // M<S5803 is set via jumper to 76 or 77
  frugal_iot.sensors->add(new Sensor_ms5803("pressure", "Pressure", 0x77));
  // system_oled and actuator_ledbuiltin added automatically on boards that have them.
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and sysetm
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop();
}

