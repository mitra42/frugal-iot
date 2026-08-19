/* 
 *  Frugal IoT example - ENS160 AHT21 environment sensor
 * 
 *  The common little "ENS160+AHT21" breakout carries two independent I2C chips, and this
 *  example adds one Frugal-IoT sensor for each:
 *
 *    Sensor_AHT21  (sensor/aht.h)    - temperature and humidity
 *    Sensor_ENS160 (sensor/ens160.h) - air quality: AQI, TVOC, eCO2
 *
 *  The ENS160 needs an ambient temperature and humidity to compensate its gas plate, and takes
 *  them as *inputs* rather than reading a sensor of its own. That is the wiring this example
 *  makes below - and since those are ordinary wireable inputs, on a node that already has (say)
 *  an SHT30, the ENS160 can be wired to that instead and the AHT21 left out altogether.
 */

#include "Frugal-IoT.h"

// Change the parameters here to match your ... 
// organization, project, id, description
System_Frugal frugal_iot(SYSTEM_FRUGAL_ORG, SYSTEM_FRUGAL_PROJECT, "ENS160 AHT21", "ENS160 AHT21 Environmental Sensor");

void setup() {
  // Battery sensor has to come before pre_setup, all others should come after
  #ifdef SENSOR_BATTERY_PIN
    frugal_iot.configure_battery(SENSOR_BATTERY_PIN); // Adds default battery sensor can specify (pin, Scale)
  #endif
  
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
  Sensor_AHT21* aht = new Sensor_AHT21("AHT21");
  frugal_iot.sensors->add(aht);
  Sensor_ENS160* ens = new Sensor_ENS160("ENS160");
  frugal_iot.sensors->add(ens);
  // Feed the AHT21's readings to the ENS160 for its compensation. Sensor_ENS160::setup() would
  // do exactly this on its own (SENSOR_ENS160_TEMPERATURE_PATH / _HUMIDITY_PATH default to the
  // AHT21's topics), but it is spelled out here because it is the point of the example - and
  // because wiring it explicitly is what you would edit to feed it from a different sensor.
  ens->temperature->wireTo(aht->temperature->path());
  ens->humidity->wireTo(aht->humidity->path());
  
  Control_Hysteresis* cb = new Control_Hysteresis("controlhysteresis", "Control", 50, 1, 0, 100);
  frugal_iot.controls->add(cb);
  cb->outputs[0]->wireTo(frugal_iot.messages->setPath("ledbuiltin/on"));

  // Dont change below here - should be after setup the actuators, controls and sensors
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and sysetm
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop(); // Should be running watchdog.loop which will call esp_task_wdt_reset()
}

