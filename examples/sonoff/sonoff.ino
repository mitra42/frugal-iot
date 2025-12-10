/* 
 *  Frugal IoT example - Sonoff - Basic R2 or R4 switch
 *
 * Optional: 
 */

#include "Frugal-IoT.h"

// Change the parameters here to match your ... 
// organization, project, device name, description
System_Frugal frugal_iot("dev", "developers", "sonoff", "Sonoff R2 switch");

enum sonoff_state_t {  SONOFF_OFF, SONOFF_AUTO, SONOFF_ON };

void setup() {
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
  // Relay on Sonoff is on pin 12
  frugal_iot.actuators->add(new Actuator_Digital("relay", "Relay", RELAY_BUILTIN, "purple"));

  ControlHysterisis* ch = new ControlHysterisis("controlhysterisis", "Control", 50, 1, 0, 100);
  frugal_iot.controls->add(ch);
  ch->outputs[0]->wireTo(frugal_iot.messages->setPath("relay/on")); // TODO refactor wireTo so can take a Base

  // https://github.com/mitra42/frugal-iot/issues/159

  Sensor_Button* button = new Sensor_Button("button", "Button", BUILTIN_BUTTON, "red");
  frugal_iot.buttons->add(button);
  frugal_iot.buttons->outputs.push_back(new OUTuint16(frugal_iot.buttons->id, "state", "State", SONOFF_OFF, SONOFF_OFF, SONOFF_ON, "black", true));
  button->longClick->wireTo(frugal_iot.messages->path("frugal-iot/reboot"));
  button->singleClick->wireTo(frugal_iot.messages->path("buttons/state/cycle"));

  // Dont change below here - should be after setup the actuators, controls and sensors
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and sysetm
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop(); // Should be running watchdog.loop which will call esp_task_wdt_reset()
}
