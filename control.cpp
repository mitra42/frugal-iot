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

class IO {
  public:
    float value;
    String const * topicpath; // Topic currently listening to for input1Value
    char const * controlleaf; // Topic to listen to, that is used to set input1Topic
    IO(float v, String const * topicpath, char const * const controlleaf);
    IO(float v, char const * const topicleaf, char const * const controlleaf);
    virtual void setup();
    void dispatchLeaf(const String &topicleaf, String &payload); // Just checks control
};

IO::IO(float v, String const * tp = NULL, char const * const cl = NULL): value(v), topicpath(tp), controlleaf(cl) { };

IO::IO(float v, char const * const tl = NULL, char const * const cl = NULL) {
  IO(v, tl ? Mqtt->topicPath(tl) : NULL, cl);
};
void IO::setup() {
    if (controlleaf) Mqtt->subscribe(controlleaf);
}

void IO::dispatchLeaf(const String &tl, String &p) {
  if (tl == controlleaf) {
    if (p != *topicpath) {
      topicpath = new String(p);
      Mqtt->subscribe(*topicpath);
      // Drop thru and return false;
    }
  }
}


class IN : public IO {
  public:
  IN(float v, String const * topicpath, char const * const controlleaf);  
  IN(float v, char const * const topicleaf, char const * const controlleaf);
  bool dispatchPath(const String &topicpath, const String &payload); // For IN checks 
  virtual void setup();
};

IN::IN(float v, String const * tp, char const * const cl): IO(v,tp,cl) {}

IN::IN(float v, char const * const tl, char const * const cl): IO(v,tl,cl) {}

void IN::setup() {
  IO::setup();
  if (topicpath) Mqtt->subscribe(*topicpath);
}

// Check incoming message, return true if value changed and should carry out actions
bool IN::dispatchPath(const String &tp, const String &p) {
  if (topicpath && (tp == *topicpath)) {
    float v = p.toFloat();
    if (v != value) {
      value = v;
      return true;
    }
  }
  return false; 
}

class OUT : public IO {
  public:
    OUT(float v, String const * topicpath, char const * const controlleaf);  
    OUT(float v, char const * const topicleaf, char const * const controlleaf);
    void set(const float newvalue);
};

OUT::OUT(float v, String const * tp = NULL, char const * const cl = NULL) : IO(v,tp,cl) { }
OUT::OUT(float v, char const * const tl = NULL, char const * const cl = NULL) : IO(v,tl,cl) { }
// OUT::setup() - note OUT does not subscribe to the topic, it only sends on the topic
// OUT::dispatch() - uses IO since wont be a topic, 

void OUT::set(const float newvalue) {
  if (newvalue != value) {
    value = newvalue;
    Mqtt->messageSend(*topicpath, value, 1, true, 1 ); // TODO note defaulting to 1DP which may or may not be appropriate, retain and qos=1 
  }
}

class Control : public Frugal_Base {
  public:
    Control();
};
Control::Control() {}

class Control_3x3x3 : public Control {
  public:
    typedef std::function<void(void)> TCallback;
    IN inputs[3]; // Array of inputs
    // TODO-25 probably better as function pointers rather than virtual, can define controls then without new classes.
    OUT outputs[3];
    TCallback actions[3]; // Array of actions
    Control_3x3x3(IN inputs[3], OUT outputs[3], TCallback actions[3]);
    void setup();
};

extern std::vector<Control*> controls;


Control_3x3x3::Control_3x3x3(IN i[], OUT o[], TCallback a[]): Control() {
  inputs = i;
  //, outputs(o), actions(a)  
  controls.push_back(this); 
}
/*
void Control_3x3x3::setup() {
  for (IO i: inputs) { i.setup();
  for (IO o: outputs) { o.setup();
}
Control_3x3x3::dispatched() {
  Split into dispatchPath and dispatchleaf for IN
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
std::vector<Control*> controls;
*/
/*
// Example definition
IN ch_in1(0, "humidity, "humidity_sensor_control");
IN ch_in2(50, "limit");
IN ch_in3(5, "hysterisis");
IN ch_ins[3] = [ch_in1,ch_in2,ch_in3];
OUT ch_out1(0, "ledbuiltin", "relay_control"); // Default to control LED, controllable via "relay_control")
OUT* ch_outs[3] = [ch_out1,NULL,NULL];
Tcallback hysterisisAction(Control_3x3x3* self) { //TODO-25 need to figure out how to access variables, maybe pass this pointer
  const float hum = self->inputs[0].value;
  const float lim = self->inputs[1].value;
  const float hysterisis = self->inputs[2].value;
  if (hum > (lim + hysterisis)) { self->outputs[0].set(1); }
  if (hum < (lim - hysterisis)) { self->outputs[0].set(0); }
  // If  lim-histerisis < hum < lim+histerisis then don't change setting
}
Tcallback ch_acts[3] = [hysterisisAction, NULL, NULL]
Control_3x3x3* control_humidity = new Control_3x3x3(ch_ins, ch_outs, ch_acts);
*/
/*
// Example for blinken 
long unsigned lastblink; // Note local variable in same contex as control_blinken
IN cb_in1 = [1,"blinkspeed",NULL];
OUT cb_out1 = [0, "ledbuiltin", NULL];
IN* cb_ins[3] = [cb_in1, NULL, NULL];
OUT* cb_outs[3] = [cb_out1, NULL, NULL];
Tcallback blink(Control_3x3x3* self) {
  if (lastblink + (self->inputs[0].value*1000)) < millis() { 
    self->outputs[0].set(!!self->outputs[0].value);
    lastblink = millis();
  }
}
Tcallback cb_acts[3] = [xxx, NULL, NULL]
Control_3x3x3* control_blinken = new Control_3xx3x3(cb_ins, cb_outs, cb_acts)
*/