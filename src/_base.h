#ifndef BASE_H
#define BASE_H

#include <Arduino.h>

class Frugal_Base {
  public:
    Frugal_Base();
    virtual void setup();
    static void setupAll();
    virtual void loop();
    static void loopAll();
    virtual void dispatch(const String &topic_msg, const String &payload);
    static void dispatchAll(const String &topic, const String &payload);
}; // Class FrugalBase

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
    //virtual void set(const float newvalue); // Similarly - setting into types from variety of values
    //virtual void set(const bool newvalue);
    virtual String advertisement(const char * const name);
};
class IN : public IO {
  public:
    IN(char const * const name, char const * const topicLeaf, char const *color, const bool wireable);
  virtual String advertisement(const char * const name);
    virtual float floatValue();
    virtual bool boolValue();
    virtual bool dispatchLeaf(const String &topicleaf, const String &payload); // Just checks control
};
class OUT : public IO {
  public:
    OUT(char const * const name, char const * const topicLeaf, char const *color, const bool wireable);
    virtual String advertisement(const char * const name);
    //virtual void set(const float newvalue); // Similarly - setting into types from variety of values
    //virtual void set(const bool newvalue);
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
    String advertisement(const char * const name);

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
    String advertisement(const char * const name);
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
    String advertisement(const char * const name);
};

#endif // BASE_H