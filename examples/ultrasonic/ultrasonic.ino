/*
 *  Frugal IoT example - ultrasonic distance sensor, any of the modules the library has a driver for
 *
 *  One sketch, three drivers, chosen by a flag in platformio.ini (README.md has the wiring for each):
 *
 *    (default)                   Sensor_HCSR04 (sensor/hcsr04.h) - trigger/echo: HC-SR04, HC-SR04P,
 *                                  or an AJ-SR04M / SR04M-2 as shipped (nothing fitted on R19)
 *    SENSOR_A01NYUB_WANT         Sensor_A01NYUB (sensor/a01nyub.h) - serial frames: DFRobot A01NYUB
 *                                  or A02YYUW, or an AJ-SR04M / SR04M-2 with 120k on R19. ESP32 only
 *    SENSOR_ULTRASONIC_SLAVE_ID  Sensor_UltrasonicModbus (sensor/ultrasonicmodbus.h) - RS485 / Modbus,
 *                                  e.g. A01ANY4B, through a TTL-RS485 converter. ESP32 only
 *
 *  All three use the same two ESP pins, SENSOR_ULTRASONIC_TRIG_PIN and SENSOR_ULTRASONIC_ECHO_PIN, set per board
 *  in platformio.ini. ECHO is always the wire coming FROM the module - an HC-SR04's ECHO, a serial
 *  module's TX, an RS485 converter's RO - so the same wire works whichever driver is built.
 *
 *  Each publishes the distance in mm. For the depth of water in a tank rather than the distance
 *  down to it, set offset to the height of the sensor above the tank floor in mm and scale to -1 -
 *  see sensor/ultrasonic.h, and the commented-out lines below.
 */

#include "Frugal-IoT.h"

// Change the parameters here to match your ...
// organization, project, id, description
System_Frugal frugal_iot(SYSTEM_FRUGAL_ORG, SYSTEM_FRUGAL_PROJECT, "Ultrasonic", "Ultrasonic Distance Sensor");

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
  #if defined(SENSOR_ULTRASONIC_SLAVE_ID)
    #ifdef ESP8266
      #error "RS485 needs a UART that can receive, and the ESP8266's only one is the console - use an ESP32 board"
    #endif
    #define ULTRASONIC_ID "ultrasonic"
    // One bus object per converter; its pins are SYSTEM_RS485_RX_PIN / _TX_PIN from platformio.ini
    System_RS485* rs485 = new System_RS485(&Serial1); // Serial1 - the C3 and S2 have no Serial2
    frugal_iot.sensors->add(new Sensor_UltrasonicModbus(ULTRASONIC_ID, "Distance",
      DEFAULT_ultrasonic_ultrasonic_max, DEFAULT_ultrasonic_ultrasonic_color, true, rs485))
      ->powerPins(SENSOR_ULTRASONIC_POWER3v3_PIN, SENSOR_ULTRASONIC_POWER0_PIN); // Switches the RS485 bus
    // Depth of water instead, for a sensor mounted 2000mm above the tank floor:
    //  new Sensor_UltrasonicModbus(ULTRASONIC_ID, "Depth", 2000, DEFAULT_ultrasonic_ultrasonic_color,
    //    true, rs485, 2000.0, -1.0);
  #elif defined(SENSOR_A01NYUB_WANT)
    #ifdef ESP8266
      #error "A serial module needs a UART that can receive, and the ESP8266's only one is the console - use an ESP32 board"
    #endif
    #define ULTRASONIC_ID "a01nyub"
    frugal_iot.sensors->add(new Sensor_A01NYUB(ULTRASONIC_ID, "Distance", &Serial1, SENSOR_ULTRASONIC_ECHO_PIN))
      ->powerPins(SENSOR_ULTRASONIC_POWER3v3_PIN, SENSOR_ULTRASONIC_POWER0_PIN);
    // Depth of water instead, for a sensor mounted 2000mm above the tank floor:
    //  new Sensor_A01NYUB(ULTRASONIC_ID, "Depth", &Serial1, SENSOR_ULTRASONIC_ECHO_PIN,
    //    2000, DEFAULT_a01nyub_a01nyub_color, true, 2000.0, -1.0);
  #else
    #define ULTRASONIC_ID "hcsr04"
    Sensor_HCSR04* hcsr04 = new Sensor_HCSR04(ULTRASONIC_ID, "Distance", SENSOR_ULTRASONIC_TRIG_PIN, SENSOR_ULTRASONIC_ECHO_PIN);
    frugal_iot.sensors->add(hcsr04)->powerPins(SENSOR_ULTRASONIC_POWER3v3_PIN, SENSOR_ULTRASONIC_POWER0_PIN);
    // Depth of water instead, for a sensor mounted 2000mm above the tank floor:
    //  new Sensor_HCSR04(ULTRASONIC_ID, "Depth", SENSOR_ULTRASONIC_TRIG_PIN, SENSOR_ULTRASONIC_ECHO_PIN,
    //    2000, DEFAULT_hcsr04_hcsr04_color, true, 2000.0, -1.0);

    // Trigger/echo times the echo, so it needs the speed of sound, which changes with air
    // temperature by about 0.17% per degree C. It assumes 20C until something is wired to its
    // temperature input - here an SHT on the same node. It can also be wired in the UX.
    // (The serial and RS485 modules correct for temperature themselves.)
    //Sensor_SHT* sht = new Sensor_SHT("SHT", SENSOR_SHT_ADDRESS, &I2C_WIRE, true);
    //frugal_iot.sensors->add(sht);
    //hcsr04->temperature->wireTo(sht->temperature->path());
  #endif

  // On a board with an OLED, show the distance on it - large, with its unit and a label
  #ifdef ACTUATOR_OLED_WANT
    Control_Oled_Single* cos = new Control_Oled_Single("Control OLED", "mm", "Distance");
    frugal_iot.controls->add(cos);
    cos->value->wireTo(frugal_iot.messages->path(ULTRASONIC_ID "/" ULTRASONIC_ID)); // The sensor's output
  #endif

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
