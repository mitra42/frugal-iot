/* 
 *  Frugal IoT example - SOIL Sensor, with SHT
 * 
 */

// defines SENSOR_SHT_ADDRESS if dont define here or in platformio.ini
#include "Frugal-IoT.h"
// Change the parameters here to match your ... 
// organization, project, device name, description
System_Frugal frugal_iot(SYSTEM_FRUGAL_ORG, SYSTEM_FRUGAL_PROJECT, "Soil", "Soil probe"); 

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
  frugal_iot.configure_power(Power_Loop, 30000, 30000); // Take a reading every 30 seconds - awake all the time

  frugal_iot.pre_setup(); // Encapsulate setting up and starting serial and read main config

  // The node fetches its OWN broker credential rather than sharing the organization's password.
  // Host and secret both come from platformio.ini; the secret belongs in the uncommitted
  // <name>-local.ini, never in a sketch. Built with no secret, the node is refused and listed on
  // the dashboard's Nodes card for an administrator to approve - that is by design.
  // The shared-password form still works - comment the last line and uncomment this to go back:
  //frugal_iot.configure_mqtt(SYSTEM_MQTT_HOST, SYSTEM_MQTT_USER, SYSTEM_MQTT_PASSWORD);
  frugal_iot.configure_mqtt_enrolled(SYSTEM_MQTT_HOST, SYSTEM_MQTT_ENROL_SECRET);

  // Soil sensor: 0% at the DRY reading, 100% at the WET one. The pin and both readings are set per
  // board in platformio.ini, as the ADC's range differs by chip. Tare and Calibrate in the captive
  // portal to set them for your probe.
  Sensor_Soil* soil = new Sensor_Soil("soil", "Soil", SENSOR_SOIL_PIN,
    SENSOR_SOIL_DRY, -100.0 / (SENSOR_SOIL_DRY - SENSOR_SOIL_WET), "brown", true);
  frugal_iot.sensors->add(soil);

  // actuator_oled and actuator_ledbuiltin added automatically on boards that have them.

  // Add local wifis here, or see instructions in the wiki for adding via the /data
  //frugal_iot.wifi->addWiFi(F("mywifissid"),F("mywifipassword"));
  
  // Add sensors, actuators and controls
  frugal_iot.sensors->add(new Sensor_SHT("SHT", SENSOR_SHT_ADDRESS, &I2C_WIRE, true));
  
  // On a board with an OLED, show the soil moisture on it - large, with its unit and a label
  #ifdef ACTUATOR_OLED_WANT
    frugal_iot.controls->add((new Control_Oled_Single("Control OLED", "%", "Soil moisture"))->wireFrom(soil));
  #endif

  // If required, add a control - this is just an example
  Control_Hysteresis* cb = new Control_Hysteresis("controlhysteresis", "Control", 50, 1, 0, 100);
  frugal_iot.controls->add(cb);
  cb->wireTo(frugal_iot.led);

  // Dont change below here - should be after setup the actuators, controls and sensors
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and sysetm
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop(); // Should be running watchdog.loop which will call esp_task_wdt_reset()
}

