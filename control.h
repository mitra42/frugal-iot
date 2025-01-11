/*
 * Generic base class for controls
 *
 * It makes some assumptions - e.g. max 3 float inputs, which if wrong may require refactoring. 
 *
 */

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include <vector>
#include "_base.h"

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

class IN : public IO {
  public:
  IN(); 

  IN(float v, String const * topicpath, char const * const controlleaf);  
  IN(float v, char const * const topicleaf, char const * const controlleaf);
  bool dispatchPath(const String &topicpath, const String &payload); // For IN checks 
  virtual void setup();
};

class OUT : public IO {
  public:
    OUT();
    OUT(float v, String const * topicpath, char const * const controlleaf);  
    OUT(float v, char const * const topicleaf, char const * const controlleaf);
    void set(const float newvalue);
};

class Control : public Frugal_Base {
  public:
    typedef std::function<void(Control*)> TCallback;
    std::vector<IN> inputs; // Vector of inputs
    std::vector<OUT> outputs; // Vector of outputs
    std::vector<TCallback> actions; // Vector of actions

    Control(std::vector<IN> i, std::vector<OUT> o, std::vector<TCallback> a);
    void setup();
    void debug(const char* const blah);
    void dispatch(const String &topicpath, const String &payload);
    static void setupAll();
    static void dispatchAll(const String &topicpath, const String &payload);
};

extern std::vector<Control*> controls;

// TODO-25 make this auto-generated
#define CONTROL_ADVERTISEMENT "\n  -\n    topic: control_input\n    name: Sensor Control Input\n    type: topic\n    options: float\n    display: dropdown\n    rw: rw" \
                                         "\n  -\n    topic: control_limit\n    name: Maximum value\n    type: float\n    min: 1\n    max: 100\n    display: slider\n    rw: rw" \
                                         "\n  -\n    topic: control_hysterisis\n    name: Plus or Minus\n    type: float\n    min: 0\n    max: 20\n    display: slider\n    rw: rw" \
                                         "\n  -\n    topic: control_output\n    name: Output to\n    type: topic\n    options: bool\n    display: dropdown\n    rw: rw"

