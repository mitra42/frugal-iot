/* 
 *  Frugal IoT example - Sonoff - Basic R2 or R4 switch
 *
 * Optional: 
 */

#include "Frugal-IoT.h"

#include "control_hysterisis.h"

//TODO-189 maybe these should just be variables not a class ? 
class Control_Sonoff : public Control_Hysterisis {
  public:
    OUTbool* manual;
    Control_Sonoff();
  protected:
    // Override dispatchTwig to handle switch of "out" when change from auto to manual.
    void dispatchTwig(const String &topicControlId, const String &topicLeaf, const String &payload, bool isSet) override;
    void act() override;
};

Control_Sonoff::Control_Sonoff() : 
  Control_Hysterisis("controlhysterisis", "Control", 50, 1, 0, 100),
  manual(new OUTbool(id, "manual", "Manual",false,"red",true)) {
  outputs.push_back(manual); // Note push_back as dont change outputs[0] being the output result.
}

void Control_Sonoff::dispatchTwig(const String &topicControlId, const String &topicTwig, const String &payload, bool isSet) {
  if (topicControlId == id) { // matches this control
    if (topicTwig == "out/cycle") {
      // If cycling state then also set to manual
      manual->set(true);
      manual->writeValueToFS(manual->id, manual->StringValue());
    }
    Control_Hysterisis::dispatchTwig(topicControlId, topicTwig, payload, isSet); // Pass on to normal handle of manual - not clear if it writes out/on to FS
  }
}

void Control_Sonoff::act() {
  // Only propogate changes to output[0] if !manual 
  if (!(manual->value)) {
    Control_Hysterisis::act();
  }
}


// Change the parameters here to match your ... 
// organization, project, device name, description
System_Frugal frugal_iot("dev", "developers", "sonoff", "Sonoff switch");

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

   // If required, add a control - this is just an example
  Control_Sonoff* cs = new Control_Sonoff();
  frugal_iot.controls->add(cs);
  cs->outputs[0]->wireTo(frugal_iot.messages->setPath("relay/on")); // TODO refactor wireTo so can take a Base
  cs->outputs[1]->wireTo(frugal_iot.messages->setPath("ledbuiltin/on")); // Turn on LED if manual (TODO-187 may want inverse) 
  // https://github.com/mitra42/frugal-iot/issues/159

  Sensor_Button* button = new Sensor_Button("button", "Button", BUILTIN_BUTTON, "red");
  frugal_iot.buttons->add(button);
  //frugal_iot.buttons->outputs.push_back(new OUTuint16(frugal_iot.buttons->id, "state", "State", SONOFF_OFF, SONOFF_OFF, SONOFF_ON, "black", true));
  button->longClick->wireTo(frugal_iot.messages->setPath("controlhysterisis/manual/cycle")); // Value sent is "1" so goes into manual
  button->singleClick->wireTo(frugal_iot.messages->setPath("controlhysterisis/out/cycle"));
  

  // Dont change below here - should be after setup the actuators, controls and sensors
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls and sysetm
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop(); // Should be running watchdog.loop which will call esp_task_wdt_reset()
}
