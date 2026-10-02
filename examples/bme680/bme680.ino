/*
 *  Frugal IoT example - BME680 environment sensor
 *
 *  One Bosch chip that measures four things, added as one Frugal-IoT sensor:
 *
 *    Sensor_BME680 (sensor/bme680.h) - temperature, humidity, pressure and gas resistance
 *
 *  It also handles the BME688, which answers to the same chip id.
 *
 *  Gas resistance (bme680/gas, in kOhm) is NOT an air quality index. It is the resistance of a
 *  heated metal-oxide plate - higher is cleaner air - but the absolute value drifts with
 *  humidity, temperature and the sensor's age, so watch the trend rather than the number.
 *
 *  The gas heater is the expensive part, about 12mA for 150ms on every read. On a battery node
 *  that only wants temperature, humidity and pressure, pass wantGas = false (see below) and the
 *  heater is never switched on.
 *
 *  Address: 0x77 by default, which is what most breakouts use. If yours has SDO tied low it is
 *  at 0x76 - pass that as the second argument, or set SENSOR_BME680_ADDRESS in platformio.ini.
 */

#include "Frugal-IoT.h"

// Change the parameters here to match your ...
// organization, project, id, description
System_Frugal frugal_iot(SYSTEM_FRUGAL_ORG, SYSTEM_FRUGAL_PROJECT, "BME680", "BME680 Environmental Sensor");

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
  // Note that on Deep the gas reading is unreliable - the heater needs several reads to settle,
  // and deep sleep starts it again on every wake. Temperature, humidity and pressure are fine.
  frugal_iot.configure_power(Power_Loop, 30000, 30000); // Take a reading every 30 seconds - awake all the time

  frugal_iot.pre_setup(); // Encapsulate setting up and starting serial and read main config
  // The node fetches its OWN broker credential rather than sharing the organization's password.
  // Host and secret both come from platformio.ini; the secret belongs in the uncommitted
  // <name>-local.ini, never in a sketch. Built with no secret, the node is refused and listed on
  // the dashboard's Nodes card for an administrator to approve - that is by design.
  // The shared-password form still works - comment the last line and uncomment this to go back:
  //frugal_iot.configure_mqtt(SYSTEM_MQTT_HOST, SYSTEM_MQTT_USER, SYSTEM_MQTT_PASSWORD);
  frugal_iot.configure_mqtt_enrolled(SYSTEM_MQTT_HOST, SYSTEM_MQTT_ENROL_SECRET);

  // Add local wifis here, or see instructions in the wiki for adding via the /data
  //frugal_iot.wifi->addWiFi(F("mywifissid"),F("mywifipassword"));

  // Add sensors, actuators and controls
  // actuator_oled and actuator_ledbuiltin added automatically on boards that have them.
  frugal_iot.sensors->add(new Sensor_BME680("BME680")); // bme680/temperature|humidity|pressure|gas
  // Without the gas heater - no bme680/gas, and much less power:
  //frugal_iot.sensors->add(new Sensor_BME680("BME680", SENSOR_BME680_ADDRESS, &I2C_WIRE, true, false));

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
