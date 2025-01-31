/*
 * Generic base class for controls
 *
 * It makes some assumptions - e.g. max 3 float inputs, which if wrong may require refactoring. 
 *
 */

#ifndef CONTROL_H
#define CONTROL_H

#include "_settings.h"  // Settings for what to include etc
#include <Arduino.h>
#include <vector>
#include "_base.h"

#if defined(CONTROL_HYSTERISIS_DEBUG) || defined(CONTROL_MPQ_DEBUG)
  #define CONTROL_DEBUG
#endif

// TODO_25 move IO...OUT to _base.h and _base.cpp
class IO {
  public:
    char const *name; // Name of this IO within the sensor, i.e. can duplicate across sensors
    char const *topicLeaf; // Topic always listens|sends to - null (unusual) if only listens on wire
    char const *color; // String passed to UX
    bool const wireable; // True if can wire this to/from others
    char const *wireLeaf; // Topic listens for change to wiredTopic, will always be wire_<sensor.name>_<io.name>
    const String *wiredPath; // Topic also listening|sending to when wired
    IO();
    IO(const char * const n, const char * const tl, char const *color, const bool w = true);
    virtual void setup(const char * const sensorname);
    virtual bool dispatchLeaf(const String &topicleaf, const String &payload); // Just checks control
    virtual bool dispatchPath(const String &topicPath, const String &payload);
    #ifdef CONTROL_DEBUG
      virtual void debug(const char* const where);
    #endif
    virtual float floatValue(); // Can build these for other types and combos e.g. returning bool from a float etc
    virtual void set(const float newvalue); // Similarly - setting into types from variety of values
    virtual String* advertisement();
};
class IN : public IO {
  public:
    IN(char const * const name, char const * const topicLeaf, char const *color, const bool wireable);
    virtual String* advertisement();
    virtual float floatValue();
    virtual bool boolValue();
    virtual bool dispatchLeaf(const String &topicleaf, const String &payload); // Just checks control
};
class OUT : public IO {
  public:
    OUT(char const * const name, char const * const topicLeaf, char const *color, const bool wireable);
    virtual String* advertisement();
    virtual float floatValue();
    virtual bool boolValue();
    virtual void sendWired();
    virtual bool dispatchLeaf(const String &topicleaf, const String &payload); // Just checks control
};
class INfloat : public IN {
  public:
    float value;
    float min;
    float max;
    INfloat(); 
    INfloat(char const * const name, float v, char const * const topicLeaf, float min, float max, char const * const color, const bool wireable);
    INfloat(const INfloat &other);
    float floatValue(); // This is so that other subclasses e.g. INuint16 can still return a float if required
    bool boolValue();

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
    void debug(const char* const where);
    String* advertisement();

};

class OUTfloat : public OUT {
  public:
    float value;
    float min;
    float max;
    OUTfloat();
    OUTfloat(char const * const name, float v, char const * const topicLeaf, float min, float max, char const * const color, const bool wireable);
    OUTfloat(const OUTfloat &other);
    float floatValue(); // This is so that other subclasses e.g. OUTuint16 can still return a float if required
    bool boolValue();
    void sendWired();
    void set(const float newvalue);
    void debug(const char* const where);
    String* advertisement();
};
class OUTbool : public OUT {
  public:
    bool value;
    OUTbool();
    OUTbool(char const * const name, bool v, char const * const topicLeaf, char const * const color, const bool wireable);
    OUTbool(const OUTbool &other);
    float floatValue(); // This is so that other subclasses e.g. OUTuint16 can still return a float if required
    bool boolValue();
    void set(const bool newvalue);
    void sendWired();
    void debug(const char* const where);
    String* advertisement();
};

class Control : public Frugal_Base {
  public:
    const char * const name;
    typedef std::function<void(Control*)> TCallback;
    std::vector<IN*> inputs; // Vector of inputs
    std::vector<OUT*> outputs; // Vector of outputs
    std::vector<TCallback> actions; // Vector of actions

    Control(const char * const name, std::vector<IN*> i, std::vector<OUT*> o, std::vector<TCallback> a); 
    void setup();
    void dispatch(const String &topicPath, const String &payload);
    String* advertisement();
    static void setupAll();
    static void loopAll();
    static void dispatchAll(const String &topicPath, const String &payload);
    static String* advertisementAll();
    #ifdef CONTROL_DEBUG
      void debug(const char* const blah);
    #endif //CONTROL_DEBUG
};

extern std::vector<Control*> controls;

// TODO-25 if auto-generation works remove this
#define CONTROL_ADVERTISEMENT "\n  -\n    topic: wire_humidity_control_humiditynow\n    name: Humidity Now\n    type: topic\n    options: float\n    display: dropdown\n    rw: w" \
                              "\n  -\n    topic: humidity_limit\n    name: Maximum value\n    type: float\n    min: 1\n    max: 100\n    display: slider\n    rw: w" \
                              "\n  -\n    topic: hysterisis\n    name: Plus or Minus\n    type: float\n    min: 0\n    max: 20\n    display: slider\n    rw: w" \
                              "\n  -\n    topic: wire_humidity_control_out\n    name: Output to\n    type: topic\n    options: bool\n    display: dropdown\n    rw: w"

#endif //CONTROL_H
