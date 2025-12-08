/* 
 *  Frugal IoT example - Data Logger
 *
 * This is a Work in Progress - it works but may not be useful yet.
 * 
 * See https//github.com/frugal-iot/issues/110 for more information.
 * 
 */

#include "Frugal-IoT.h"
// Change the parameters here to match your ... 
// organization, project, id, description
System_Frugal frugal_iot("dev", "developers", "datalogger", "Data Logger");

void setup() {
  frugal_iot.pre_setup(); // Encapsulate setting up and starting serial and read main config

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

  // actuator_oled and actuator_ledbuiltin added automatically on boards that have them.

  // Add local wifis here, or see instructions in the wiki for adding via the /data
  frugal_iot.wifi->addWiFi(F("mywifissid"),F("mywifipassword"));
  
  // Add sensors, actuators and controls
  // Temperature and Humidity sensor (SHT30)
  Sensor_SHT* sht;
  frugal_iot.sensors->add(sht = new Sensor_SHT("SHT", SENSOR_SHT_ADDRESS, &I2C_WIRE, true));
  
  // Controls that can be wire here, or in the UX
  // Add time if needed, which is currently only for data logging.
  frugal_iot.system->add(frugal_iot.time = new System_Time());
  #ifdef SYSTEM_SD_WANT
    System_SD* fs_SD = new System_SD(SYSTEM_SD_PIN);
    frugal_iot.system->add(fs_SD);
  #else // If no SD then use LittleFS
    // LittleFS is always added - for configuration
    /*
    System_LittleFS* fs_LittleFS = new System_LittleFS();
    frugal_iot.system->add(fs_LittleFS);
    */
  #endif

  // Must be after sensor_sht for default wiring below
  Control_Logger* clfs = new Control_LoggerFS(
    "Logger",
    #ifdef SYSTEM_SD_WANT
      fs_SD, // Use SD card for logging
    #else
      frugal_iot.fs_LittleFS, // TODO-110 Using LittleFS for testing for now
    #endif
    "/",
    0x02, // Single log.csv with topicPath, time, value
    std::vector<IN*> {
      //INtext(const char * const sensorId, const char * const id, const char* const name, String* value, const char* const color, const bool wireable)
      new INtext("Logger", "log1", "log1", String(), "black", true),
      new INtext("Logger", "log2", "log2", String(), "black", true),
      new INtext("Logger", "log3", "log3", String(), "black", true)
      });
  frugal_iot.controls->add(clfs);
  // Wire the logger to the temperature sensor, it could be left blank and wired in the UX to a remote sensor
  clfs->inputs[0]->wireTo(sht->temperature->path()); // Wired to the temperatur sensor 

  // Dont change below here - should be after setup the actuators, controls and sensors
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and sysetm
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop(); // Should be running watchdog.loop which will call esp_task_wdt_reset()
}

