/* 
 *  Frugal IoT example - ENS160 AHT21 environment sensor
 * 
 */

#include "Frugal-IoT.h"

// Change the parameters here to match your ... 
// organization, project, id, description
System_Frugal frugal_iot("dev", "developers", "ENS160 AHT21", "ENS160 AHT21 Environmental Sensor");

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

  // Add local wifis here, or see instructions in the wiki for adding via the /data
  //frugal_iot.wifi->addWiFi(F("mywifissid"),F("mywifipassword"));
  
  // Add sensors, actuators and controls
  // actuator_oled and actuator_ledbuiltin added automatically on boards that have them.
  frugal_iot.sensors->add(new Sensor_ensaht("ensaht","ENS160 AHT21"));
  
  ControlHysterisis* cb = new ControlHysterisis("controlhysterisis", "Control", 50, 1, 0, 100);
  frugal_iot.controls->add(cb);
  cb->outputs[0]->wireTo(frugal_iot.messages->path("ledbuiltin/on"));

  // Dont change below here - should be after setup the actuators, controls and sensors
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and sysetm
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop(); // Should be running watchdog.loop which will call esp_task_wdt_reset()
}

