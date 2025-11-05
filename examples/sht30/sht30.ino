/* 
 *  Frugal IoT example - SHT30 temperature and humidity sensor
 * 
 * Optional: SENSOR_SHT_ADDRESS - defaults to 0x44, (note the D1 shields default to 0x45)
 */

// defines SENSOR_SHT_ADDRESS if dont define here or in platformio.ini
#include "Frugal-IoT.h"

#ifdef SYSTEM_OLED_WANT
  #include "control_oled_sht.h" // Custom display handler
#endif

// Change the parameters here to match your ... 
// organization, project, device name, description
System_Frugal frugal_iot("dev", "developers", "SHT30", "SHT30 Temperature and Humidity Sensor"); 

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
  frugal_iot.sensors->add(new Sensor_SHT("SHT", SENSOR_SHT_ADDRESS, &I2C_WIRE, true));
  
  //frugal_iot.sensors->add(new Sensor_Battery());

  // If required, add a control - this is just an example
  //ControlHysterisis* cb = new ControlHysterisis("controlhysterisis", "Control", 50, 1, 0, 100);
  //frugal_iot.controls->add(cb);
  //cb->outputs[0]->wireTo(frugal_iot.messages->path("ledbuiltin/on"));

  #ifdef SYSTEM_OLED_WANT
    Control_Oled_SHT* cos = new Control_Oled_SHT("Control OLED");
    frugal_iot.controls->add(cos);
    cos->temperature->wireTo(frugal_iot.messages->path("sht/temperature"));
    cos->humidity->wireTo(frugal_iot.messages->path("sht/humidity"));  
    cos->battery->wireTo(frugal_iot.messages->path("battery/battery"));
  #endif

  // Dont change below here - should be after setup the actuators, controls and sensors
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and sysetm
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop(); // Should be running watchdog.loop which will call esp_task_wdt_reset()
}

