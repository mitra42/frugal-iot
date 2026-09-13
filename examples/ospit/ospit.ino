/*
 *  Frugal IoT example - OSPIT irrigation
 *
 *  A port of the irrigation half of OSPIT (https://github.com/mitra42/ospit) - the solar-powered
 *  irrigation controller built on the FF-ESP32-OpenMPPT board. It waters a set of sectors one at
 *  a time, once a day, each until its soil reaches a target moisture or a time limit runs out,
 *  with tank-level and battery interlocks that stop the whole run.
 *
 *  What is here, and what is not:
 *    - irrigation, soil probes, tank gauge, valves, pump          YES
 *    - MPPT solar charge control                                  NO - that is a later phase.
 *      This build leaves the FF board's charge hardware entirely alone and uses the board as a
 *      plain ESP32. On the FF board, the existing charge controller is simply not driven.
 *
 *  Two boards, see platformio.ini: `ff_openmppt` (the real OSPIT hardware) and `s2_mini` (the
 *  same application on a plain dev board with no charge-controller hardware).
 *
 *  ============================================================================================
 *  IRRIGATION IS OFF BY DEFAULT.
 *
 *  `irrigation/enabled` starts false, as OSPIT's i_nbld does - a device that opens water valves
 *  unattended should not begin doing so merely because it was flashed. Turn it on once, from the
 *  captive portal or by publishing to `set/<device>/irrigation/enabled` = 1; the setting is
 *  written to LittleFS and survives reboots and deep sleep.
 *
 *  Worth setting at the same time, all persisted the same way:
 *    irrigation/hour, irrigation/minute   when the daily run starts, local time (default 03:00)
 *    irrigation/maxminutes                longest any one valve may stay open (default 5)
 *    sectorN/target                       moisture % at which sector N is satisfied (default 80)
 *    controlhysteresis/hysteresis         see the battery interlock note below
 *  ============================================================================================
 */

#include "Frugal-IoT.h"
#include "control_irrigation.h"
#include "sensor_tank.h"

// Change the parameters here to match your ...
// organization, project, device name, description
System_Frugal frugal_iot(SYSTEM_FRUGAL_ORG, SYSTEM_FRUGAL_PROJECT, "ospit", "OSPIT Irrigation");

void setup() {
  // Battery sensor has to come before pre_setup, all others should come after
  #ifdef SENSOR_BATTERY_PIN
    frugal_iot.configure_battery(SENSOR_BATTERY_PIN);
  #endif

  /* Awake all the time, on a 10 second cycle.
   *
   * Deliberately NOT a sleeping mode. Valve timing has the resolution of one wake cycle, and more
   * importantly a deep sleep in the middle of a run would abandon the run - Control_Irrigation
   * returns to idle in setup(). Control_Irrigation::allowSleep() is the hook a sleep manager will
   * use to avoid exactly that; nothing calls it yet.
   */
  frugal_iot.configure_power(Power_Loop, 10000, 10000);

  frugal_iot.pre_setup();

  // Override MQTT host, username and password if you have an "organization" other than "dev"
  frugal_iot.configure_mqtt("frugaliot.naturalinnovation.org", "dev", "public");

  // Add local wifis here, or see instructions in the wiki for adding via the /data
  //frugal_iot.wifi->addWiFi(F("mywifissid"),F("mywifipassword"));

  // ---- Actuators: one valve per sector, plus the pump -----------------------------------
  // On the FF board the pump pin is also the main load output and OSPIT's low-voltage disconnect
  // switch - three jobs on one pin. Here it is only the pump; if your board shares it, wire the
  // other users to `pump/on` rather than driving the pin behind this actuator's back.
  frugal_iot.actuators->add(new Actuator_Digital("valve1", "Valve 1", OSPIT_VALVE1_PIN, DEFAULT_valve1_on_color));
  frugal_iot.actuators->add(new Actuator_Digital("valve2", "Valve 2", OSPIT_VALVE2_PIN, DEFAULT_valve2_on_color));
  frugal_iot.actuators->add(new Actuator_Digital("valve3", "Valve 3", OSPIT_VALVE3_PIN, DEFAULT_valve3_on_color));
  frugal_iot.actuators->add(new Actuator_Digital("pump", "Pump", OSPIT_PUMP_PIN, DEFAULT_pump_on_color));

  // ---- Sensors -------------------------------------------------------------------------
  // One RS485 bus, one probe per sector, slave ids 1..3. A probe that does not answer publishes
  // "nan", which is what makes Control_Irrigation skip that sector - the same job OSPIT's -127
  // does, but without doubling as the switch that turns an output into a USB socket.
  System_RS485* rs485 = new System_RS485(&OSPIT_RS485_UART);
  frugal_iot.sensors->add(new Sensor_SoilModbus("soil1", "Sector 1 probe", 1, rs485, true));
  frugal_iot.sensors->add(new Sensor_SoilModbus("soil2", "Sector 2 probe", 2, rs485, true));
  frugal_iot.sensors->add(new Sensor_SoilModbus("soil3", "Sector 3 probe", 3, rs485, true));

  // Resistive float sender in the tank. Publishes "nan" if no sender is fitted, which is NOT
  // treated as an empty tank - see sensor_tank.h.
  frugal_iot.sensors->add(new Sensor_Tank("tank", "Water tank", OSPIT_TANK_PIN, true));

  // ---- Battery interlock ----------------------------------------------------------------
  /* Reproduces OSPIT's low_voltage_disconnect: stop irrigating when the battery is too low.
   *
   * Battery readings are millivolts, so 12100 is 12.1 V - between OSPIT's 11.9 V disconnect and
   * 12.3 V reconnect. `greater` defaults true, so `out` is true (irrigation permitted) above the
   * limit.
   *
   * TODO the dead band is 0 here, i.e. one bare threshold, so a battery hovering at 12.1 V under
   * a pump load will chatter. OSPIT's 11.9/12.3 pair is a hysteresis of 200 mV, which belongs in
   * `controlhysteresis/hysteresis` - but INfloat has no public setter, so an example cannot set a
   * control's starting hysteresis from code. Set it once from the portal or by publishing
   * `set/<device>/controlhysteresis/hysteresis` = 200; it persists. A public setter on INfloat
   * would remove the need and is a library change worth making.
   */
  Control_Hysteresis* ch = new Control_Hysteresis("controlhysteresis", "Battery interlock", 12100, 0, 10000, 15000);
  frugal_iot.controls->add(ch);
  ch->inputs[0]->wireTo(frugal_iot.messages->path("battery/battery"));

  // ---- Irrigation ------------------------------------------------------------------------
  Control_Irrigation* irr = new Control_Irrigation("irrigation", "Irrigation");
  frugal_iot.controls->add(irr);
  irr->tank->wireTo(frugal_iot.messages->path("tank/tank"));
  irr->pump->wireTo(frugal_iot.messages->setPath("pump/on"));
  ch->outputs[0]->wireTo(frugal_iot.messages->setPath("irrigation/power"));

  // Sectors run in the order they are added. addSector() registers each one as a control in its
  // own right, so each gets its own portal section, MQTT topics and discovery.
  Control_Sector* s1 = irr->addSector("sector1", "Sector 1");
  s1->moisture->wireTo(frugal_iot.messages->path("soil1/humidity"));
  s1->valve->wireTo(frugal_iot.messages->setPath("valve1/on"));

  Control_Sector* s2 = irr->addSector("sector2", "Sector 2");
  s2->moisture->wireTo(frugal_iot.messages->path("soil2/humidity"));
  s2->valve->wireTo(frugal_iot.messages->setPath("valve2/on"));

  Control_Sector* s3 = irr->addSector("sector3", "Sector 3");
  s3->moisture->wireTo(frugal_iot.messages->path("soil3/humidity"));
  s3->valve->wireTo(frugal_iot.messages->setPath("valve3/on"));

  // Dont change below here - should be after setup the actuators, controls and sensors
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and system
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop(); // Should be running watchdog.loop which will call esp_task_wdt_reset()
}
