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

#if defined(CONTROL_HYSTERISIS_DEBUG) || defined(CONTROL_MPQ_DEBUG)
  #define CONTROL_DEBUG
#endif

class IO {
  public:
    char const *name; // Name of this IO within the sensor, i.e. can duplicate across sensors
    char const *topicLeaf; // Topic always listens|sends to - null (unusual) if only listens on wire
    bool const wireable; // True if can wire this to/from others
    char const *wireLeaf; // Topic listens for change to wiredTopic, will always be wire_<sensor.name>_<io.name>
    const String *wiredPath; // Topic also listening|sending to when wired
    IO();
    IO(const char * const n, const char * const tl = nullptr, const bool w = true);
    virtual void setup(const char * const sensorname);
    virtual bool dispatchLeaf(const String &topicleaf, const String &payload); // Just checks control
    virtual bool dispatchPath(const String &topicPath, const String &payload);
    #ifdef CONTROL_DEBUG
      virtual void debug(const char* const where);
    #endif
    virtual float floatValue(); // Can build these for other types and combos e.g. returning bool from a float etc
    virtual void set(const float newvalue); // Similarly - setting into types from variety of values
};
class IOfloat : public IO {
  public:
    float value;
    IOfloat(char const * const name, float v, char const * const topicLeaf = nullptr, const bool wireable = true);
    float floatValue();
    void debug(const char* const where);
};
class INfloat : public IOfloat {
  public:
    INfloat(); 
    INfloat(char const * const name, float v, char const * const topicLeaf = nullptr, const bool wireable = true);
    INfloat(const INfloat &other);
    // Copy assignment operator
    /*
    INfloat& operator=(const INfloat &other) {
      Serial.print("XXXXXX IN assignment __FILE__"); Serial.println(__LINE__); // Debugging here, because dont think this is used
      if (this != &other) {
          value = other.value;
          name = other.name;
          wireable = other.wireable
          topicLeaf = other.topicLeaf;
          wireLeaf = other.wireLeaf;
          wiredPath = other.wiredPath;
      }
      return *this;
    }
    */
    bool dispatchLeaf(const String &topicLeaf, const String &payload);
    bool dispatchPath(const String &topicPath, const String &payload);
    virtual void setup(const char * const sensorname);
};

class OUTfloat : public IOfloat {
  public:
    OUTfloat();
    OUTfloat(char const * const name, float v, char const * const topicLeaf = nullptr, const bool wireable = true);
    OUTfloat(const OUTfloat &other);
    void set(const float newvalue);
};

class Control : public Frugal_Base {
  public:
    const char * const name;
    typedef std::function<void(Control*)> TCallback;
    std::vector<IO*> inputs; // Vector of inputs // TODO-25B change to
    std::vector<IO*> outputs; // Vector of outputs  // TODO-25B change to
    std::vector<TCallback> actions; // Vector of actions

    Control(const char * const name, std::vector<IO*> i, std::vector<IO*> o, std::vector<TCallback> a); 
    void setup();
    void dispatch(const String &topicPath, const String &payload);
    static void setupAll();
    static void dispatchAll(const String &topicPath, const String &payload);
    #ifdef CONTROL_DEBUG
      void debug(const char* const blah);
    #endif //CONTROL_DEBUG
};

extern std::vector<Control*> controls;

// TODO-25 make this auto-generated and move to control_hysterisis.h
#define CONTROL_ADVERTISEMENT "\n  -\n    topic: wire_humidity_control_humiditynow\n    name: Humidity Now\n    type: topic\n    options: float\n    display: dropdown\n    rw: rw" \
                              "\n  -\n    topic: humidity_limit\n    name: Maximum value\n    type: float\n    min: 1\n    max: 100\n    display: slider\n    rw: rw" \
                              "\n  -\n    topic: hysterisis\n    name: Plus or Minus\n    type: float\n    min: 0\n    max: 20\n    display: slider\n    rw: rw" \
                              "\n  -\n    topic: wire_humidity_control_out\n    name: Output to\n    type: topic\n    options: bool\n    display: dropdown\n    rw: rw"

