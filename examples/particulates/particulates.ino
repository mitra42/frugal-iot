/*
 *  Frugal IoT example - particulate matter, with a humidity sensor beside it. README.md has the wiring.
 *
 *  The particulate sensor is chosen in platformio.ini - each board has an env for each:
 *    SENSOR_SDS011_WANT  Nova Fitness SDS011, as in sensor.community's airRohr - on a UART.
 *                        Publishes sds011/pm25 and sds011/pm10 in ug/m3.
 *    SENSOR_HM3301_WANT  Seeed Grove HM3301 - on I2C.
 *                        Publishes hm3301/pm1, hm3301/pm25 and hm3301/pm10 in ug/m3.
 *
 *  Runs on deep sleep. The sensor holds the node awake until its reading is done, so wake_ms below
 *  only needs to cover everything else. The SDS011 is woken each cycle, so each wake runs its fan
 *  for a 15s warm-up, then averages 5s of readings and puts it back to sleep. The HM3301 cannot
 *  sleep - it runs all the time - so after its first warm-up each wake just averages 5s of
 *  readings. See sensor/pm.h, sensor/sds011.h, sensor/hm3301.h.
 *
 *  The humidity sensor is chosen in platformio.ini - whichever of these is defined, first wins:
 *    SENSOR_DHT_PIN      DHT22 (or DHT11) on that pin
 *    SENSOR_SHT_WANT     SHT3x / SHT4x on I2C
 *    SENSOR_BME280_WANT  BME280 on I2C (the default, as in the airRohr kit)
 *  It matters because both PM sensors count water droplets as particles, so readings above ~70%
 *  relative humidity are too high. Nothing here corrects for that - it is published so it can be.
 */

#include "Frugal-IoT.h"

#if defined(SENSOR_SDS011_WANT) && !defined(ESP32)
  #error "The SDS011 needs a UART that can receive, and the ESP8266's only one is the console - use an ESP32 board"
#endif
#if !(defined(SENSOR_SDS011_WANT) || defined(SENSOR_HM3301_WANT))
  #error "No particulate sensor - define SENSOR_SDS011_WANT or SENSOR_HM3301_WANT (platformio.ini does, per env)"
#endif

// Change the parameters here to match your ...
// organization, project, id, description
System_Frugal frugal_iot(SYSTEM_FRUGAL_ORG, SYSTEM_FRUGAL_PROJECT, "PM", "Particulate matter");

void setup() {
  // Battery sensor has to come before pre_setup, all others should come after
  #ifdef SENSOR_BATTERY_PIN
    frugal_iot.configure_battery(SENSOR_BATTERY_PIN); // Adds default battery sensor can specify (pin, Scale)
  #endif

  // Configure power handling - type, cycle_ms, wake_ms
  // A reading every 5 minutes. Awake at least 10s, and longer while the sensor is measuring.
  // For a node on mains, or while debugging, Power_Loop keeps it awake - the SDS011 still sleeps
  // between readings when the cycle is longer than a measurement (the HM3301 never does).
  // Note for 3 minutes after power-on (not after a wake) the node stays awake so its access point
  // can be found - see System_Captive.
  frugal_iot.configure_power(Power_Deep, 300000, 10000);

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

  // The particulate sensor - see the top of this file
  #if defined(SENSOR_SDS011_WANT)
    // Serial1 because the C3 has no Serial2; any UART works on the others.
    frugal_iot.sensors->add(new Sensor_SDS011("PM", &Serial1, SENSOR_SDS011_RX_PIN, SENSOR_SDS011_TX_PIN));
  #elif defined(SENSOR_HM3301_WANT)
    // On the I2C bus with the BME280 or SHT. Not given powerPins() - see sensor/hm3301.h.
    frugal_iot.sensors->add(new Sensor_HM3301("PM"));
  #endif

  // The humidity sensor - see the top of this file. Its 3V3 comes from a GPIO, *_POWER3v3_PIN,
  // so it is off while the node sleeps; the pins are in platformio.ini and README.md.
  #if defined(SENSOR_DHT_PIN)
    frugal_iot.sensors->add(new Sensor_DHT("DHT", SENSOR_DHT_PIN, true))
      ->powerPins(SENSOR_DHT_POWER3v3_PIN, SENSOR_DHT_POWER0_PIN);
  #elif defined(SENSOR_SHT_WANT)
    frugal_iot.sensors->add(new Sensor_SHT("SHT", SENSOR_SHT_ADDRESS, &I2C_WIRE, true))
      ->powerPins(SENSOR_SHT_POWER3v3_PIN, SENSOR_SHT_POWER0_PIN);
  #elif defined(SENSOR_BME280_WANT)
    frugal_iot.sensors->add(new Sensor_BME280("BME280")) // Address SENSOR_BME280_ADDRESS, default 0x76
      ->powerPins(SENSOR_BME280_POWER3v3_PIN, SENSOR_BME280_POWER0_PIN);
  #endif

  // Dont change below here - should be after setup the actuators, controls and sensors
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and sysetm
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop(); // Should be running watchdog.loop which will call esp_task_wdt_reset()
}
