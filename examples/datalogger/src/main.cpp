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
  frugal_iot.system->add(frugal_iot.time = new System_Time());
  #ifdef SYSTEM_SD_WANT
    System_SD* fs_SD = new System_SD();
    frugal_iot.system->add(fs_SD);
  #else // If no SD then use SPIFFS
    System_SPIFFS* fs_SPIFFS = new System_SPIFFS(); // TODO-141 merge with SPIFFS used by WiFi
    frugal_iot.system->add(fs_SPIFFS);
  #endif
  Sensor_SHT* ss = new Sensor_SHT("SHT", SENSOR_SHT_ADDRESS, &Wire, true); // Create SHT30 sensor
  frugal_iot.sensors->add(ss); // Add SHT30 sensor

  // Must be after sensor_sht for default wiring below
  // TODO-141 Make match pattern
  Control_Logger* clfs = new Control_LoggerFS(
    "Logger",
    #ifdef SYSTEM_SD_WANT
      fs_SD, // Use SD card for logging
    #else
      fs_SPIFFS, // TODO-110 Using spiffs for testing for now
    #endif
    "/",
    0x02, // Single log.csv with topicPath, time, value
    std::vector<IN*> {
      //INtext(const char * const sensorId, const char * const id, const char* const name, String* value, const char* const color, const bool wireable)
      new INtext("Logger", "log1", "log1", nullptr, "black", true),
      new INtext("Logger", "log2", "log2", nullptr, "black", true),
      new INtext("Logger", "log3", "log3", nullptr, "black", true)
      });
  frugal_iot.controls->add(clfs);
  // Wire the logger to the temperature sensor, it could be left blank and wired in the UX to a remote sensor
  clfs->inputs[0]->wireTo(ss->temperature); // Wired to the temperatur sensor
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and system
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop();
}

