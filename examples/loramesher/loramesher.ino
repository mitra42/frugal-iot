/* 
 *  Frugal IoT example - LoRaMesher demo - a work in progress
 * 
 * Optional: 
 * 
 */

#include "Frugal-IoT.h"
#include "system_mqtt.h"

// Change the parameters here to match your ... 
// organization, project, id, description
System_Frugal frugal_iot("dev", "developers", "loramesher", "LoraMesher Node");

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
  frugal_iot.wifi->addWiFi(F("mywifissid"),F("mywifipassword"));
  
  //esp_log_level_set(LM_TAG, ESP_LOG_INFO);     // enable INFO logs from LoraMesher - but doesnt seem to work
  frugal_iot.loramesher = new System_LoraMesher(); // Held in a variable as future LoRaMesher will access it directly e.g. from MQTT
  frugal_iot.system->add(frugal_iot.loramesher);
  
  // Add sensors, actuators and controls
  #ifdef SYSTEM_LORAMESHER_SENDER
    frugal_iot.sensors->add(new Sensor_SHT("SHT", SENSOR_SHT_ADDRESS, &I2C_WIRE, true));
  #endif 
  // system_oled and actuator_ledbuiltin added automatically on boards that have them.
  // Dont change below here - should be after setup the actuators, controls and sensors
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and sysetm
  Serial.println(F("FrugalIoT Starting Loop"));
}

#ifdef SYSTEM_LORAMESHER_DEBUG
void printAppData() {
    frugal_iot.oled->display.clearDisplay();
    frugal_iot.oled->display.setCursor(0,0);
    frugal_iot.oled->display.print(frugal_iot.description);
    frugal_iot.oled->display.setCursor(0,10);
    frugal_iot.oled->display.print(frugal_iot.loramesher->checkRoleString());
    if (frugal_iot.loramesher->lastTopicPath) {
      // Display information
      frugal_iot.oled->display.setCursor(0,20);
      frugal_iot.oled->display.print("last packet:");
      frugal_iot.oled->display.setCursor(0,30);
      frugal_iot.oled->display.print(frugal_iot.loramesher->lastTopicPath);
      frugal_iot.oled->display.print(":");
      frugal_iot.oled->display.print(frugal_iot.loramesher->lastPayload);      
    }
    frugal_iot.oled->display.display();   
}
#endif


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

