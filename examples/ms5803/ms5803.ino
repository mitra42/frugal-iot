/* 
 *  Frugal IoT example - MS5803 pressure sensor
 * 
 * Note that code appears to work, but testing gets nonsense readings - maybe a faulty device
 * 
 */

#include "frugal_iot.h"

// Change the parameters here to match your ... 
// organization, project, id, description
System_Frugal frugal_iot("dev", "developers", "ms5803", "MS5803 Pressure sensor");

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

  // system_oled and actuator_ledbuiltin added automatically on boards that have them.

  // Add local wifis here, or see instructions in the wiki for adding via the /data
  //frugal_iot.wifi->addWiFi(F("mywifissid"),F("mywifipassword"));

  // Add sensors, actuators and controls
    
  // MS5803 is set via jumper to 76 or 77
  frugal_iot.sensors->add(new Sensor_ms5803("ms5803", "MS5803", 0x77));
  
  // Dont change below here - should be after setup the actuators, controls and sensors
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and sysetm
  Serial.println(F("FrugalIoT Starting Loop"));
}

// You can put custom code in here, 
void loop() {
  if (frugal_iot.timeForPeriodic) {
    // Things which happen once for each sensor read period go here.  
    // But note, you do not have to put sensor loops etc here - the loop and periodic functions in
    // each sensor and actuator and control are called from frugal_iot.loop()
    // This is also a good place to put things that check how long since last running
  }
  frugal_iot.loop(); // Do not delete this call to frugal_iot.loop
}

