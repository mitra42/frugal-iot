/*
 *  Frugal IoT example - BMP280 AHT20 environment sensor
 *
 *  The common little "AHT20+BMP280" breakout carries two independent I2C chips, and this
 *  example adds one Frugal-IoT sensor for each:
 *
 *    Sensor_AHT20  (sensor/aht.h)    - temperature and humidity
 *    Sensor_BMP280 (sensor/bmx280.h) - pressure (and temperature, see below)
 *
 *  Both chips publish a temperature, but use the AHT20's. The BMP280 measures temperature only
 *  to compensate its own pressure reading, and it is inaccurate as an ambient temperature.
 *  Sensor_BMP280 publishes it anyway, as bmp280/temperature - there is no way yet to leave one
 *  of a sensor's outputs unpublished - so just ignore it (or leave it off any dashboard).
 *
 *  Unlike the ENS160 on the ENS160+AHT21 board (see examples/ensaht), neither chip needs the
 *  other's readings for compensation, so there is no wiring between them - they are just two
 *  sensors that happen to share a board.
 *
 *  Addresses: the AHT20 is at 0x38, its only address. On this breakout the BMP280 has SDO tied
 *  high, putting it at 0x77 rather than the library's default of 0x76 - so it is given
 *  explicitly below. If yours does not respond, try 0x76.
 */

#include "Frugal-IoT.h"

// Change the parameters here to match your ...
// organization, project, id, description
System_Frugal frugal_iot(SYSTEM_FRUGAL_ORG, SYSTEM_FRUGAL_PROJECT, "BMP280 AHT20", "BMP280 AHT20 Environmental Sensor");

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

  // Add local wifis here, or see instructions in the wiki for adding via the /data
  //frugal_iot.wifi->addWiFi(F("mywifissid"),F("mywifipassword"));

  // Add sensors, actuators and controls
  // actuator_oled and actuator_ledbuiltin added automatically on boards that have them.
  frugal_iot.sensors->add(new Sensor_AHT20("AHT20"));          // aht20/temperature|humidity
  frugal_iot.sensors->add(new Sensor_BMP280("BMP280", 0x77));  // bmp280/pressure (and bmp280/temperature - ignore it)

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
