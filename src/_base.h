#ifndef BASE_H
#define BASE_H

#include <Arduino.h>

extern const char* valueAdvertLineFloat;
extern const char* valueAdvertLineBool;
extern const char* wireAdvertLine;

// TO-ADD-INXXX TO-ADD-OUTXXX
enum IOtype { 
  BOOL, UINT16, FLOAT, COLOR
};


class Frugal_Base {
  public:
    Frugal_Base();
    virtual void setup();
    static void setupAll();
    virtual void loop();
    static void loopAll();
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
    void wireTo(String* topicPath);
};
class IN : public IO {
  public:
    IN(char const * const name, char const * const topicLeaf, char const *color, const bool wireable);
    virtual String advertisement(const char * const name);
    // TO-ADD-INxxx
    virtual float floatValue();
    virtual bool boolValue();
    virtual uint16_t uint16Value();
    virtual bool convertAndSet(const String &payload);
    virtual bool dispatchLeaf(const String &topicleaf, const String &payload);
    virtual bool dispatchPath(const String &topicpath, const String &payload); 
    void setup(const char*);
};
class OUT : public IO {
  public:
    OUT(char const * const name, char const * const topicLeaf, char const *color, const bool wireable);
    virtual String advertisement(const char * const name);
    //virtual void set(const float newvalue); // Similarly - setting into types from variety of values
    //virtual void set(const bool newvalue);
    // TO-ADD-OUTxxx
    virtual float floatValue();
    virtual bool boolValue();
    virtual uint16_t uint16Value();
    virtual void sendWired();
    virtual bool dispatchLeaf(const String &topicleaf, const String &payload); // Just checks control
};

// TO-ADD-INxxx
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
    uint16_t uint16Value();

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
    bool convertAndSet(const String &payload);
    void debug(const char* const where);
    String advertisement(const char * const name);
};
class INuint16 : public IN {
  public:
    uint16_t value;
    uint16_t min;
    uint16_t max;
    INuint16(); 
    INuint16(char const * const name, uint16_t v, char const * const topicLeaf, uint16_t min, uint16_t max, char const * const color, const bool wireable);
    INuint16(const INuint16 &other);
    float floatValue(); // This is so that other subclasses e.g. INuint16 can still return a float if required
    bool boolValue();
    uint16_t uint16Value();
    bool convertAndSet(const String &payload);
    void debug(const char* const where);
    String advertisement(const char * const name);

};
class INbool : public IN {
  public:
    bool value;
    //INbool(); 
    INbool(char const * const name, bool value, char const * const topicLeaf, char const * const color, const bool wireable);
    INbool(const INuint16 &other);
    float floatValue(); // This is so that other subclasses e.g. INuint16 can still return a float if required
    bool boolValue();
    uint16_t uint16Value();
    bool convertAndSet(const String &payload);
    void debug(const char* const where);
    String advertisement(const char * const name);
};

class INcolor : public IN {
  public:
    uint8_t r;
    uint8_t g;
    uint8_t b;
    INcolor(); 
    INcolor(char const * const name, uint8_t r, uint8_t g, uint8_t b,  char const * const topicLeaf, const bool wireable);
    INcolor(const INcolor &other);
    float floatValue(); // This is so that other subclasses e.g. INuint16 can still return a float if required
    bool boolValue();
    uint16_t uint16Value();
    bool convertAndSet(const String &payload);
    void debug(const char* const where);
    String advertisement(const char * const name);
};

// TO-ADD-OUTxxx
class OUTfloat : public OUT {
  public:
    float value;
    uint8_t width;
    float min;
    float max;
    OUTfloat();
    OUTfloat(char const * const name, float v, char const * const topicLeaf, uint8_t width, float min, float max, char const * const color, const bool wireable);
    OUTfloat(const OUTfloat &other);
    float floatValue(); // This is so that other subclasses e.g. OUTuint16 can still return a float if required
    bool boolValue();
    uint16_t uint16Value();
    void sendWired();
    void set(const float newvalue); // Set and send if changed
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
    uint16_t uint16Value();
    void set(const bool newvalue);
    void sendWired();
    void debug(const char* const where);
    String advertisement(const char * const name);
};
class OUTuint16 : public OUT {
  public:
    uint16_t value;
    uint16_t min;
    uint16_t max;
    OUTuint16();
    OUTuint16(char const * const name, uint16_t v, char const * const topicLeaf, uint16_t mn, uint16_t mx, char const * const color, const bool wireable);
    OUTuint16(const OUTuint16 &other);
    float floatValue(); // This is so that other subclasses e.g. OUTuint16 can still return a float if required
    bool boolValue();
    uint16_t uint16Value();
    void set(const uint16_t newvalue);
    void sendWired();
    void debug(const char* const where);
    String advertisement(const char * const name);
};

#endif // BASE_H