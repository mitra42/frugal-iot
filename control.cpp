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
    IO();
    IO(float v, String const * topicpath, char const * const controlleaf);
    IO(float v, char const * const topicleaf, char const * const controlleaf);
    virtual void setup();
    void dispatchLeaf(const String &topicleaf, const String &payload); // Just checks control
};
IO::IO() {}

IO::IO(float v, String const * tp = NULL, char const * const cl = NULL): value(v), topicpath(tp), controlleaf(cl) { };

IO::IO(float v, char const * const tl = NULL, char const * const cl = NULL) {
  IO(v, tl ? Mqtt->topicPath(tl) : NULL, cl);
};
void IO::setup() {
    if (controlleaf) Mqtt->subscribe(controlleaf);
}

void IO::dispatchLeaf(const String &tl, const String &p) {

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
  IN(); 

  IN(float v, String const * topicpath, char const * const controlleaf);  
  IN(float v, char const * const topicleaf, char const * const controlleaf);
  bool dispatchPath(const String &topicpath, const String &payload); // For IN checks 
  virtual void setup();
};

IN::IN() {}

IN::IN(float v, String const * tp = nullptr, char const * const cl = nullptr): IO(v,tp,cl) {}

IN::IN(float v, char const * const tl = nullptr, char const * const cl = nullptr): IO(v,tl,cl) {}

void IN::setup() {
  IO::setup();
  if (topicpath) Mqtt->subscribe(*topicpath);
}

// Note also has dispatchLeaf via the superclass
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
    OUT();
    OUT(float v, String const * topicpath, char const * const controlleaf);  
    OUT(float v, char const * const topicleaf, char const * const controlleaf);
    void set(const float newvalue);
};
OUT::OUT() {};
OUT::OUT(float v, String const * tp = NULL, char const * const cl = NULL) : IO(v,tp,cl) { }
OUT::OUT(float v, char const * const tl = NULL, char const * const cl = NULL) : IO(v,tl,cl) { }
// OUT::setup() - note OUT does not subscribe to the topic, it only sends on the topic
// OUT::dispatchLeaf() - uses IO since wont be incoming topicpath, only a controlleaf

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
    typedef std::function<void(Control_3x3x3*)> TCallback;
    IN inputs[3]; // Array of inputs
    OUT outputs[3]; // Array of outputs
    TCallback actions[3]; // Array of actions
    Control_3x3x3(IN i[3], OUT o[3], TCallback a[3]);
    void setup();
    void dispatch(const String &topicpath, const String &payload );
};

extern std::vector<Control*> controls;

Control_3x3x3::Control_3x3x3(IN i[3], OUT o[3], TCallback a[3]) {
  for (int j = 0; j < 3; ++j) {
    inputs[j] = i[j];
    outputs[j] = o[j];
    actions[j] = a[j];
  }
  controls.push_back(this); 
}

void Control_3x3x3::setup() {
  for (int j = 0; j < 3; ++j) {
      inputs[j].setup();
      outputs[j].setup();
  }
  // Try this
 // for (IO i: inputs) { i.setup();
 // for (IO o: outputs) { o.setup();
}
void Control_3x3x3::dispatch(const String &topicpath, const String &payload ) {
  bool changed = false;
  String* tl = Mqtt->topicLeaf(topicpath);
  for (int j = 0; j < 3; ++j) {
    if (tl) { // Will be nullptr if no match
      // Both inputs and outputs have possible 'control' and therefore dispatchLeaf
      inputs[j].dispatchLeaf(*tl, payload); // Does not trigger any messages or actions - though response to subscription will.
      inputs[j].dispatchLeaf(*tl, payload); // TODO-25 Setting a topic *should* but doesnt yet send a message to new outout.topic
    }
    // Presuming only inputs get incoming messages to the topic
    if (inputs[j].dispatchPath(topicpath, payload)) { 
      changed = true; // Changed an input, do the actions
    }
  }
  if (changed) {
    for (int j = 0; j < 3; ++j) {
      if (actions[j]) {
        actions[j](this);        // Actions should call self.outputs[x].set(newvalue); and allow .set to check if changed and send message
      }
    }
  }
}

std::vector<Control*> controls;

// Example definition
IN ch_in1(0, "humidity", "humidity_sensor_control");
IN ch_in2(50, "limit");
IN ch_in3(5, "hysterisis");
IN ch_ins[3] = {ch_in1, ch_in2, ch_in3};

OUT ch_out1(0, "ledbuiltin", "relay_control"); // Default to control LED, controllable via "relay_control")
OUT ch_out2; // Default constructor
OUT ch_out3;
OUT ch_outs[3] = {ch_out1, ch_out2, ch_out3};

Control_3x3x3::TCallback hysterisisAction = [](Control_3x3x3* self) {
    const float hum = self->inputs[0].value;
    const float lim = self->inputs[1].value;
    const float hysterisis = self->inputs[2].value;
    if (hum > (lim + hysterisis)) {
        self->outputs[0].set(1);
    }
    if (hum < (lim - hysterisis)) {
        self->outputs[0].set(0);
    }
  // If  lim-histerisis < hum < lim+histerisis then don't change setting
};
Control_3x3x3::TCallback actions[3] = {hysterisisAction, nullptr, nullptr};

Control_3x3x3 control_humidity(ch_ins, ch_outs, actions);
/*
// Example for blinken  - TODO-25 note needs a loop for timing
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