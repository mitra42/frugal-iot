/*
 * Generic base class for controls
 *
 * It makes some assumptions - e.g. max 3 float inputs, which if wrong may require refactoring. 
 *
 */

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include <vector>
#include "control.h"
#include "system_mqtt.h"

// TODO-ADD-CONTROL
#if defined(CONTROL_XYZ_DEBUG) || defined(CONTROL_MPQ_DEBUG)
  #define CONTROL_DEBUG
#endif

std::vector<Control*> controls;

class Control : public Frugal_Base {
  public:
}
class IO {
    float value;
    String const * topic; // Topic currently listening to for input1Value
    String const * const control; // Topic to listen to, that is used to set input1Topic
    IO(float v, String const * t = NULL, String const * const c = NULL);
    virtual void setup();
    void dispatch(String &topic, String &payload); // Just checks control
}
IO::IO(float v, String const * t = NULL, String const * const c = NULL): value(v), topic(t), control(c) {
  topic = xMqtt::topic_expand(topic);
  control = xMqtt::topic_expand(control);
};
IO::setup() {
    if (control) Mqtt->subscribe(control);
}

void IO::dispatch(String &t, String &p) {
  if (control == t) {
    if (*p != *topic) {
      topic = p;
      Mqtt->subscribe(topic);
      // Drop thru and return false;
    }
  }
}
class IN : IO {
  IN(float v, String const * t = NULL, String const * const c = NULL);  
  bool dispatched(String &topic, String &payload); // For IN checks 
};
IN::IN(float v, String const * t = NULL, String const * const c = NULL) : IO(v,t,c) {}
IN::setup() : {
  IO::setup()
  if (topic) Mqtt->subscribe(topic);
}
// Check incoming message, return true if value changed and should carry out actions
bool IN::dispatched(String &t, String &p) {
  IO:dispatch(t, p)) { // check control
  if (topic == t) { // Check if shoul be *topic == *t
    float v = p.toFloat();
    if (v != value) {
      value = v;
      return true;
    }
  }
  return false; 
}


class OUT : IO {
  OUT(float v, String const * t = NULL, String const * const c = NULL);  
};
OUT::OUT(float v, String const * t = NULL, String const * const c = NULL) : IO(v,t,c) {}
// OUT::setup() - note OUT does not subscribe to the topic, it only sends on the topic
// OUT::dispatch() - uses IO since wont be a topic, 
class Control_3x3x3 : public Control {
  public:
    typedef std::function<void(void)> TCallback;
    IO inputs[3]; // Array of inputs
    // TODO-25 probably better as function pointers rather than virtual, can define controls then without new classes.
    Tcallback actions[3]; // Array of actions
    IO outputs[3];
    Control_3x3x3(IO inputs[3], IO outputs[3], Tcallback actions[3]);
    setup();
}
OUT::set(float newvalue) {
  if (newvalue != value) {
    value == newvalue
    Mqtt->send(topic, value);
  }
}
Control_3x3x3::Control_3x3x3(IO i[3], IO o[3], Tcallback a[3]): Control, inputs(i), outputs(o), actions(a) { 
  controls.push_back(this); 
}
Control_3x3x3::setup() {
  for (IO i: inputs) { i.setup();
  for (IO o: outputs) { o.setup();
}
Control_3x3x3::dispatched() {
  for (IO i: inputs) { if (i.dispatched()) { return true; }; // value changed
  for (IO o: outputs) { o.dispatch(); }
  return false; 
  //TODO-25 actions based on dispatch
}
Control_3x3x3::dispatch() {
  if (disptached()) {
    for (a: actions) {
      if (a) {
        a(this);        // Actions should call self.outputs[x].set(newvalue); and allow .set to check if changed and send message
      }
    }
  }
}

// Example definition
IO in1(0, "humidity, "humidity_sensor_control");
IO in2(50, "limit");
IO in3(5, "hysterisis");
IO inputs[3] = [in1,in2,in3];
IO out1(0, "ledbuiltin", "relay_control"); // Default to control LED, controllable via "relay_control")
IO* outputs[3] = [out1,NULL,NULL];
Tcallback hysterisisAction(Control_3x3x3* self) { //TODO-25 need to figure out how to access variables, maybe pass this pointer
  const float hum = self->inputs[0].value;
  const float lim = self->inputs[1].value;
  const float histerisis = self->inputs[2].value;
  if (hum > (lim + histerisis)) { self->outputs[0].set(1); }
  if (hum < (lim - histerisis)) { self->outputs[0].set(0); }
  // If  lim-histerisis < hum < lim+histerisis then don't change setting
}
actions[3] = [hysterisisAction, NULL, NULL]
Control_3x3x3* humcontroller = new Control_3x3x3(inputs, outputs, actions);
