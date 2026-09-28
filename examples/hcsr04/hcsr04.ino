/*
 *  Frugal IoT example - HC-SR04 ultrasonic distance sensor
 *
 *    Sensor_HCSR04 (sensor/hcsr04.h) - distance in mm, publishing hcsr04/hcsr04
 *
 *  WIRING - the standard HC-SR04 is a 5V part:
 *    VCC  -> 5V (VBUS / 5V pin on the dev board)
 *    GND  -> GND
 *    TRIG -> SENSOR_HCSR04_TRIGGER_PIN - 3.3V drive is fine
 *    ECHO -> SENSOR_HCSR04_ECHO_PIN through a divider, because ECHO drives 5V and the ESP pins
 *            are not rated for it: 1k from ECHO to the pin, and 2k from the pin to GND.
 *  The pins for each board are in platformio.ini (on a D1 mini that is D5 and D6).
 *  An HC-SR04P runs at 3.3V: power it from 3V3 and leave the divider out.
 *
 *  Each reading takes five pings about 60ms apart and publishes the median of the ones that came
 *  back, or "nan" if none did. The range is 20mm to 4m.
 *
 *  The speed of sound changes with air temperature, about 0.17% per degree C, so the sensor has a
 *  temperature input. It assumes 20C until something is wired to it. See below for wiring it to
 *  another sensor; it can also be wired in the UX.
 *
 *  For the depth of water in a tank rather than the distance down to it, set offset to the height
 *  of the sensor above the tank floor in mm and scale to -1 - see sensor/ultrasonic.h.
 */

#include "Frugal-IoT.h"

// Change the parameters here to match your ...
// organization, project, id, description
System_Frugal frugal_iot(SYSTEM_FRUGAL_ORG, SYSTEM_FRUGAL_PROJECT, "HCSR04", "HC-SR04 Ultrasonic Distance Sensor");

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
  Sensor_HCSR04* hcsr04 = new Sensor_HCSR04("hcsr04", "Distance", SENSOR_HCSR04_TRIGGER_PIN, SENSOR_HCSR04_ECHO_PIN);
  frugal_iot.sensors->add(hcsr04);
  // Depth of water instead, for a sensor mounted 2000mm above the tank floor:
  //  new Sensor_HCSR04("hcsr04", "Depth", SENSOR_HCSR04_TRIGGER_PIN, SENSOR_HCSR04_ECHO_PIN,
  //    2000, DEFAULT_hcsr04_hcsr04_color, true, 2000.0, -1.0);

  // Correct the speed of sound from a temperature sensor - here an SHT on the same node:
  //Sensor_SHT* sht = new Sensor_SHT("SHT", SENSOR_SHT_ADDRESS, &I2C_WIRE, true);
  //frugal_iot.sensors->add(sht);
  //hcsr04->temperature->wireTo(sht->temperature->path());

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
