/*
 *  Frugal IoT example - Rain Gauge
 *
 * Reads an LM393-based analog rain board (e.g. YL-83/FC-37) and accumulates a rain total.
 * See lib/Frugal-IoT/src/sensor/rain.h for the accumulation approach and its limits - in
 * short: it assumes the analog reading is linear with rainfall rate, which is a
 * simplification, not a calibrated fact.
 *
 * Required: SENSOR_RAIN_PIN - which analog pin the board's AO is wired to; board-specific,
 * set in platformio.ini (not every analog-capable pin works on every board - see notes in
 * sensor/analog.cpp).
 *
 * Once running: tare (dry) and calibrate (against a manual gauge, in mm) via the captive
 * portal or by publishing to the "rain/output" MQTT topic.
 */

#include "Frugal-IoT.h"

#ifndef SENSOR_RAIN_PIN
  #error "SENSOR_RAIN_PIN must be defined in platformio.ini for this board - see notes in sensor/analog.cpp on which pins support ADC"
#endif
// Can put a default here, or override in platformio.ini - will be overridden later by calibration
#ifndef SENSOR_RAIN_SCALE
  #define SENSOR_RAIN_SCALE 0.01 // Just at first start - this will be calibrated against a manual gauge
#endif

// Change the parameters here to match your ...
// organization, project, id, description
System_Frugal frugal_iot("dev", "developers", "rain", "Rain Gauge");

void setup() {
  // Battery sensor has to come before pre_setup, all others should come after
  #ifdef SENSOR_BATTERY_PIN
    frugal_iot.configure_battery(SENSOR_BATTERY_PIN); // Adds default battery sensor can specify (pin, Scale)
  #endif

  // Configure power handling - type, cycle_ms, wake_ms
  // Power_Loop = awake all the time; see other examples for Light/Deep notes.
  // Note Sensor_Rain's accumulator is a plain member, not RTC_DATA_ATTR, so it will not
  // survive Power_Deep's restarts - fine for Power_Loop, revisit if that changes.
  frugal_iot.configure_power(Power_Loop, 10000, 10000); // Read every 10 seconds - awake all the time

  // Encapsulate setting up and starting serial and read main config also checks power ok.
  // This has to happen AFTER battery and power are setup, and before mqtt and adding sensors etc.
  frugal_iot.pre_setup();

  // Override MQTT host, username and password if you have an "organization" other than "dev" (developers)
  frugal_iot.configure_mqtt("frugaliot.naturalinnovation.org", "dev", "public");

  // Add local wifis here, or see instructions in the wiki for adding via the /data
  //frugal_iot.wifi->addWiFi(F("mywifissid"),F("mywifipassword"));

  // max=500 assumes 500mm/day is a reasonable upper bound for the accumulated total between tares
  frugal_iot.sensors->add(new Sensor_Rain("rain", "Rain", SENSOR_RAIN_PIN, 500.0, SENSOR_RAIN_SCALE, DEFAULT_rain_rain_color, true));

  // actuator_oled and actuator_ledbuiltin added automatically on boards that have them.
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and sysetm
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop(); // Should be running watchdog.loop which will call esp_task_wdt_reset()
}
