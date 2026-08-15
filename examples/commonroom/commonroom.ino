/* 
 *  Frugal IoT example - Agriculture sensor - soil and air, temperature and humidity. 
 * 
 */

#include "Frugal-IoT.h"

// Define defaults for these pins here as building sensor here rather than dedicated class
#ifndef SENSOR_PH_POWER3v3_PIN
  #define SENSOR_PH_POWER3v3_PIN PIN_NONE
#endif
#ifndef SENSOR_TDS_POWER3v3_PIN
  #define SENSOR_TDS_POWER3v3_PIN PIN_NONE
#endif

// Change the parameters here to match your ... 
// organization, project, device name, description
System_Frugal frugal_iot(SYSTEM_FRUGAL_ORG, SYSTEM_FRUGAL_PROJECT, "Common Ground", "Common Ground"); 

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
  // frugal_iot.configure_power(Power_Deep, 600000, 30000); // Take a reading every 10 mins deep sleep between
  frugal_iot.configure_power(Power_Loop, 30000, 30000); // Reading every 30 seconds, no sleep
  
  // Encapsulate setting up and starting serial and read main config also checks power ok.
  // This has to happen AFTER battery and power are setup, and before mqtt and adding sensors actuators etc. 
  frugal_iot.pre_setup();

  // Override MQTT host, username and password if you have an "organization" other than "dev" (developers)
  frugal_iot.configure_mqtt("frugaliot.naturalinnovation.org", "dev", "public");


  // actuator_oled and actuator_ledbuiltin added automatically on boards that have them.

  // Add local wifis here, or see instructions in the wiki for adding via the /data
  //frugal_iot.wifi->addWiFi(F("mywifissid"),F("mywifipassword"));
  
  // Add sensors, actuators and controls
  #ifdef SENSOR_DHT_PIN
    frugal_iot.sensors->add(new Sensor_DHT("DHT", SENSOR_DHT_PIN, true))
    ->powerPins(SENSOR_DHT_POWER3v3_PIN, SENSOR_DHT_POWER0_PIN);
  #endif
  #ifdef SENSOR_DS18B20_PIN
    frugal_iot.sensors->add(new Sensor_DS18B20("ds18b20", "Water Temperature", SENSOR_DS18B20_PIN, 0, true))
    ->powerPins(SENSOR_DS18B20_POWER3v3_PIN, SENSOR_DS18B20_POWER0_PIN);
  #endif
  #ifdef SENSOR_TDS_PIN
    frugal_iot.sensors->add(new Sensor_Analog("tds", "TDS", SENSOR_TDS_PIN, 2, 0, 100, SENSOR_TDS_OFFSET, SENSOR_TDS_SCALE, "purple", true))
      ->powerPins(SENSOR_TDS_POWER3v3_PIN, PIN_NONE);
  #endif 
  #ifdef SENSOR_DO_PIN
    // Dissolved oxygen - publishes mg/L. Its water-temperature input wires itself to
    // SENSOR_DO_TEMPERATURE_PATH (default "ds18b20/ds18b20") unless already wired via
    // the filesystem or the UX, so there is nothing to wire here.
    frugal_iot.sensors->add(new Sensor_DissolvedOxygen("do", "Dissolved Oxygen", SENSOR_DO_PIN));
  #endif
  #ifdef SENSOR_INA219_WANT
    // Current/voltage/power over I2C. Publishes shunt(mV), bus(V), current(mA), power(mW)
    // and load(V) = bus + shunt/1000. NOTE current and power are only right if
    // SENSOR_INA219_SHUNT_OHMS matches the resistor actually on the board.
    frugal_iot.sensors->add(new Sensor_INA219("ina219", "Power Monitor"));
  #endif
  #ifdef SENSOR_BME280_WANT
    // Temperature, humidity and pressure over I2C (SDA/SCL from I2C_SDA/I2C_SCL).
    // Address defaults to SENSOR_BME280_ADDRESS (0x76), override for an SDO-high board.
    frugal_iot.sensors->add(new Sensor_BME280("BME280"));
  #endif
  #ifdef SENSOR_ULTRASONIC_SLAVE_ID
    // One System_RS485 per physical transceiver - pins come from SYSTEM_RS485_* in
    // platformio.ini. Share this same object with any other Modbus sensors on the bus,
    // they just use different slave ids.
    System_RS485* rs485 = new System_RS485(&Serial2);
    // Publishes offset + raw * scale, where raw is the module's reading in mm - so the
    // defaults below publish plain distance in mm. For depth of water instead, pass
    // offset = height of the sensor above the floor in mm and scale = -1.0,
    // e.g. (..., rs485, 2000.0, -1.0) for a sensor 2m up.
    frugal_iot.sensors->add(new Sensor_Ultrasonic("ultrasonic", "Water Level", 7500, "blue", true, rs485));
  #endif
  #ifdef SENSOR_PH_PIN
    // Raw ADC -> pH as (raw - offset) * scale, both from platformio.ini
    frugal_iot.sensors->add(new Sensor_Analog("ph", "PH", SENSOR_PH_PIN, 1, 0, 15, SENSOR_PH_OFFSET, SENSOR_PH_SCALE, "orange", true))
      ->powerPins(SENSOR_PH_POWER3v3_PIN, PIN_NONE);
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

