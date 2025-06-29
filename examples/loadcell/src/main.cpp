/* 
 *  Frugal IoT example - SHT30 temperature and humidity sensor
 * 
 * Optional: SENSOR_SHT_ADDRESS - defaults to 0x44, (note the D1 shields default to 0x45)
 */

// defines SENSOR_SHT_ADDRESS if dont define here or in platformio.ini
#include "frugal_iot.h"

System_Frugal frugal_iot; // Singleton

void setup() {
  frugal_iot.startSerial(); // Encapsulate setting up and starting serial
  // Add a new sensor max=2000, color="pink", retain=true, DOUTpin=0, SCKpin=1, times=10, offset=0, scale=2000
  frugal_iot.sensors->add(new Sensor_LoadCell("loadcell", "Load Cell", 2000, "pink", true,
    1, 0, 10, 0, 2000)); // DOUT, SCK, times, offset, scale
  // TODO-134 add a pair of buttons here hooked up to tare and calibrate
  // system_oled and actuator_ledbuiltin added automatically on boards that have them.
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and sysetm
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop();
}

