/*
 *  Frugal IoT example - SHT30 or SHT40 temperature and humidity sensor
 *
 * Optional: SENSOR_SHT_ADDRESS - defaults to auto-select
 */

// defines SENSOR_SHT_ADDRESS if dont define here or in platformio.ini
#include "Frugal-IoT.h"

#ifdef ACTUATOR_OLED_WANT
  #include "control_oled_ht.h" // Custom display handler
#endif
// Change the parameters here to match your ... 
// organization, project, id, description
System_Frugal frugal_iot(SYSTEM_FRUGAL_ORG, SYSTEM_FRUGAL_PROJECT, "sht30", "SHT30 Sensor");

void setup() {
  // Battery sensor has to come before pre_setup, all others should come after
  #ifdef SENSOR_BATTERY_PIN
    frugal_iot.configure_battery(SENSOR_BATTERY_PIN); // Adds default battery sensor can specify (pin, Scale)
  #endif
  
  // Configure power handling - type, cycle_ms, wake_ms 
  // power will be awake wake_ms then for the rest of cycle_ms be in a mode defined by type 
  // Loop= awake all the time; 
  // Light = Light Sleep; 
  // LightWiFi=Light + WiFi on (not working); 
  // Modem=Modem sleep - works but negligable power saving
  // Deep - works but slow recovery and slow response to UX so do not use except for multi minute cycles. 
  // frugal_iot.configure_power(Power_Loop, 30000, 30000); // Take a reading every 30 seconds - awake all the time
  frugal_iot.configure_power(Power_Deep, 600000, 30000); // Take a reading every 10 mins deep sleep between

  // Encapsulate setting up and starting serial and read main config also checks power ok.
  // This has to happen AFTER battery and power are setup, and before mqtt and adding sensors actuators etc.
  frugal_iot.pre_setup();
  // The node fetches its OWN broker credential rather than sharing the organization's password.
  // Host and secret both come from platformio.ini; the secret belongs in the uncommitted
  // <name>-local.ini, never in a sketch. Built with no secret, the node is refused and listed on
  // the dashboard's Nodes card for an administrator to approve - that is by design.
  // The shared-password form still works - comment the last line and uncomment this to go back:
  //frugal_iot.configure_mqtt(SYSTEM_MQTT_HOST, SYSTEM_MQTT_USER, SYSTEM_MQTT_PASSWORD);
  frugal_iot.configure_mqtt_enrolled(SYSTEM_MQTT_HOST, SYSTEM_MQTT_ENROL_SECRET);


  // actuator_oled and actuator_ledbuiltin added automatically on boards that have them.

  // Add local wifis here, or see instructions in the wiki for adding via the /data
  //frugal_iot.wifi->addWiFi(F("mywifissid"),F("mywifipassword"));

  // Add sensors, actuators and controls
  frugal_iot.sensors->add(new Sensor_SHT("SHT", SENSOR_SHT_ADDRESS, &I2C_WIRE, true))
    -> powerPins(SENSOR_SHT_POWER3v3_PIN, SENSOR_SHT_POWER0_PIN);
  // If required, add a control - this is just an example
  Control_Hysteresis* cb = new Control_Hysteresis("controlhysteresis", "Control", 50, 1, 0, 100);
  frugal_iot.controls->add(cb);
  cb->outputs[0]->wireTo(frugal_iot.messages->setPath("ledbuiltin/on"));

  #ifdef ACTUATOR_OLED_WANT
    Control_Oled_HT* cos = new Control_Oled_HT("Control OLED");
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

