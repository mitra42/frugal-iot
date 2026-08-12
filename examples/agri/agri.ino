/* 
 *  Frugal IoT example - Agriculture sensor - soil and air, temperature and humidity. 
 * 
 */

#include "Frugal-IoT.h"

// Change the parameters here to match your ... 
// organization, project, device name, description
System_Frugal frugal_iot(SYSTEM_FRUGAL_ORG, SYSTEM_FRUGAL_PROJECT, "Agri", "Agri Sensor"); 

void setup() {
  // Battery sensor has to come before pre_setup, all others should come after
  #ifdef SENSOR_BATTERY_PIN
    frugal_iot.configure_battery(SENSOR_BATTERY_PIN); // Adds default battery sensor can specify (pin, Scale)
  #endif

    // Configure power handling - type, cycle_ms, wake_ms 
  // power will be awake wake_ms then for the rest of cycle_ms be in a mode defined by type 
  // Power_Loop= awake all the time; 
  // Light = Light Sleep;  Good for frequent measurements e.g. every 30 seconds as usually keeps WiFi & MQTT alive
  // Power_LightWiFi=Light + WiFi on (not working); 
  // Power_Modem=Modem sleep - works but negligable power saving
  // Power_Deep - works but slow recovery and slow response to UX so do not use except for multi minute cycles. 
  frugal_iot.configure_power(Power_Deep, 600000, 30000); // Take a reading every 10 mins deep sleep between
  //frugal_iot.configure_power(Power_Loop, 10000, 10000); // For debugging sensors - 10 second loop
  
  // Encapsulate setting up and starting serial and read main config also checks power ok.
  // This has to happen AFTER battery and power are setup, and before mqtt and adding sensors actuators etc. 
  frugal_iot.pre_setup();

  // Override MQTT host, username and password if you have an "organization" other than "dev" (developers)
  frugal_iot.configure_mqtt("frugaliot.naturalinnovation.org", "dev", "public");


  // actuator_oled and actuator_ledbuiltin added automatically on boards that have them.

  // Add local wifis here, or see instructions in the wiki for adding via the /data
  //frugal_iot.wifi->addWiFi(F("mywifissid"),F("mywifipassword"));
  
  // Add sensors, actuators and controls
  frugal_iot.sensors->add(new Sensor_SHT("SHT", SENSOR_SHT_ADDRESS, &I2C_WIRE, true))->powerPins(SENSOR_SHT_POWER3v3_PIN, SENSOR_SHT_POWER0_PIN);
  #ifdef SENSOR_DS18B20_PIN
    frugal_iot.sensors->add(new Sensor_DS18B20("ds18b20", "Soil Temperature", SENSOR_DS18B20_PIN, 0, true))->powerPins(SENSOR_DS18B20_POWER3v3_PIN, SENSOR_DS18B20_POWER0_PIN);
  #endif
  #ifdef SENSOR_SOIL_PIN
    frugal_iot.sensors->add(new Sensor_Soil("soil", "Soil",SENSOR_SOIL_PIN, 4095, -100.0/4095, "brown", true))->powerPins(SENSOR_SOIL_POWER3v3_PIN, SENSOR_SOIL_POWER0_PIN);
  #endif
  // If required, add a control - this is just an example
  //Control_Hysteresis* cb = new Control_Hysteresis("controlhysteresis", "Control", 50, 1, 0, 100);
  //frugal_iot.controls->add(cb);
  //cb->outputs[0]->wireTo(frugal_iot.messages->setPath("ledbuiltin/on"));

  // Dont change below here - should be after setup the actuators, controls and sensors
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and sysetm
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop(); // Should be running watchdog.loop which will call esp_task_wdt_reset()
}

