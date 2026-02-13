/*
 *  Frugal IoT example - Climate Control
 *
 * Demonstrates dual-channel hysteresis control:
 *   SHT sensor temperature -> Control_Climate -> heating relay (pin D1)
 *   SHT sensor humidity    -> Control_Climate -> humidifier relay (pin D2)
 *
 * The control turns ON when the reading falls below (setpoint - hysteresis)
 * and OFF when it rises above (setpoint + hysteresis). Default behavior is
 * heating + humidifying; for cooling/dehumidifying, invert the relay wiring.
 *
 * Optional: SENSOR_SHT_ADDRESS - defaults to 0x44 (D1 shields use 0x45)
 */

#include "Frugal-IoT.h"
#include "control_climate.h"

// Pin definitions - override in platformio.ini if needed
// TODO these probably belong in platformio.ini
#ifndef HEATING_PIN
  #ifdef ESP8266
    #define HEATING_PIN D1
  #else
    #define HEATING_PIN 2
  #endif
#endif
#ifndef HUMIDIFIER_PIN
  #ifdef ESP8266
    #define HUMIDIFIER_PIN D2
  #else
    #define HUMIDIFIER_PIN 3
  #endif
#endif

// Change the parameters here to match your ...
// organization, project, device name, description
System_Frugal frugal_iot("dev", "developers", "climate", "Climate Control");

void setup() {
  frugal_iot.pre_setup(); // Encapsulate setting up and starting serial and read main config

  // Override MQTT host, username and password if you have an "organization" other than "dev" (developers)
  frugal_iot.configure_mqtt("frugaliot.naturalinnovation.org", "dev", "public");

  // Configure power handling - type, cycle_ms, wake_ms
  frugal_iot.configure_power(Power_Loop, 30000, 30000); // Take a reading every 30 seconds

  // Add local wifis here, or see instructions in the wiki for adding via the /data
  //frugal_iot.wifi->addWiFi(F("mywifissid"),F("mywifipassword"));

  // ========= Sensors ==============
  // Temperature and Humidity sensor (SHT30)
  Sensor_SHT* sht = new Sensor_SHT("SHT", SENSOR_SHT_ADDRESS, &I2C_WIRE, true);
  frugal_iot.sensors->add(sht);

  // ========= Actuators ==============
  // Heating relay on HEATING_PIN
  frugal_iot.actuators->add(new Actuator_Digital("heating", "Heating", HEATING_PIN, "red"));
  // Humidifier relay on HUMIDIFIER_PIN
  frugal_iot.actuators->add(new Actuator_Digital("humidifier", "Humidifier", HUMIDIFIER_PIN, "blue"));

  // ========= Controls ==============
  // Climate control: temp_setpoint=22C, temp_hysteresis=1C, humidity_setpoint=50%, humidity_hysteresis=5%
  // Args: id, name, temp_setpoint(C), temp_hysteresis(C), humidity_setpoint(%), humidity_hysteresis(%)
  Control_Climate* cc = new Control_Climate("climate", "Climate Control", 22.0, 1.0, 50.0, 5.0);
  frugal_iot.controls->add(cc);

  // Wire sensor outputs to control inputs
  // Control_Climate has 6 inputs: [0]=temperature, [1]=temp_setpoint, [2]=temp_hysteresis,
  //                                [3]=humidity,    [4]=humidity_setpoint, [5]=humidity_hysteresis
  // We wire [0] and [3] (the live readings); setpoints and hysteresis use constructor defaults
  cc->inputs[0]->wireTo(sht->temperature->path());  // SHT temperature -> climate temperature input
  cc->inputs[3]->wireTo(sht->humidity->path());      // SHT humidity -> climate humidity input

  // Wire control outputs to actuator inputs
  // Control_Climate has 2 outputs: [0]=temp_out (heating), [1]=humidity_out (humidifier)
  cc->outputs[0]->wireTo(frugal_iot.messages->setPath("heating/on"));     // temp_out -> heating relay
  cc->outputs[1]->wireTo(frugal_iot.messages->setPath("humidifier/on"));  // humidity_out -> humidifier relay

  // Dont change below here - should be after setup the actuators, controls and sensors
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and sysetm
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop(); // Should be running watchdog.loop which will call esp_task_wdt_reset()
}
