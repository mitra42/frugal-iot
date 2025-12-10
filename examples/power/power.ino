/* 
 *  Frugal IoT example - Development around power
 * 
 * Note this example will be deleted when power control is working fine and integrated
 * See https://github.com/mitra42/frugal-iot/issues/23
 * 
 */

#include "Frugal-IoT.h"
// Change the parameters here to match your ... 
// organization, project, device name, description
System_Frugal frugal_iot("dev", "developers", "Power", "Power control development"); 

void setup() {
  frugal_iot.pre_setup(); // Encapsulate setting up and starting serial and read main config

  // Override MQTT host, username and password if you have an "organization" other than "dev" (developers)
  frugal_iot.configure_mqtt("frugaliot.naturalinnovation.org", "dev", "public");

  // See https://github.com/mitra42/frugal-iot/wiki/Frugal%E2%80%90IoT:-Power-reduction for explanation
  //   frugal_iot.configure_power(Power_Loop, 60000, 10000); 
  //   frugal_iot.configure_power(Power_LightWiFi, 20000, 10000); 
  frugal_iot.configure_power(Power_Deep, 70000, 60000); 

  // actuator_oled and actuator_ledbuiltin added automatically on boards that have them.

  // Add local wifis here, or see instructions in the wiki for adding via the /data
  //frugal_iot.wifi->addWiFi(F("mywifissid"),F("mywifipassword"));
  
  // Add sensors, actuators and controls
  //frugal_iot.sensors->add(new Sensor_SHT("SHT", SENSOR_SHT_ADDRESS, &I2C_WIRE, true));
  
  // If required, add a control - this is just an example
  // TODO-23 test Control_Blinken 
  Control_Blinken* cb = new Control_Blinken("controlblinken", "Control Blinken", 5, 1);
  frugal_iot.controls->add(cb);
  cb->outputs[0]->wireTo(frugal_iot.messages->setPath("ledbuiltin/on"));

  // Dont change below here - should be after setup the actuators, controls and sensors
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and sysetm
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop(); // Should be running watchdog.loop which will call esp_task_wdt_reset()
}

