#ifndef BASE_H
#define BASE_H

#include <Arduino.h>
#include <FS.h>    // ~/Documents/Arduino/hardware/esp8266com/esp8266/cores/esp8266/FS.h

extern const char* valueAdvertLineFloat;
extern const char* valueAdvertLineBool;
extern const char* wireAdvertLine;

// Not used yet
/*
// TO-ADD-INXXX TO-ADD-OUTXXX
enum IOtype { 
  BOOL, UINT16, FLOAT, COLOR, TEXT
};
*/

class System_Base {
  public:
    const char* id = nullptr; // Name of actuator, sensor or control 
    const char* name = nullptr; // Name of actuator, sensor or control
    System_Base(const char * const id, const char * const name);
    virtual void setup();
    virtual void dispatchTwig(const String &topicActuatorId, const String &topicLeaf, const String &payload, bool isSet);
    virtual void dispatchPath(const String &topicPath, const String &payload);
    virtual String advertisement();
    void readConfigFromFS();
    void readConfigFromFS(File dir, const String* leaf);
    void writeConfigToFS(const String& topicTwig, const String& payload);
    virtual void loop();
    virtual void periodically();
    virtual void infrequently();
}; // Class FrugalBase

class IO {
  public:
    // Note that topicTwig = sensorId / id
    char const *sensorId; // Sensor this IO belongs to
    char const *id; // System readable id
    char const *name; // Human readable name of this IO within the sensor, i.e. can duplicate across sensors
    char const *topicTwig; // Topic always listens|sends to - null (unusual) if only listens on wire
    char const *color; // String passed to UX
    bool const wireable; // True if can wire this to/from others
    const String *wiredPath; // Topic also listening|sending to when wired
    IO();
    IO(const char * const sensorId, const char * const id, const char * const name, char const *color, const bool w = true);
    virtual void setup(const char * const sensorname);
    virtual bool dispatchLeaf(const String &topicLeaf, const String &payload, bool isSet); // Just checks control
    virtual bool dispatchPath(const String &topicPath, const String &payload);
    #ifdef CONTROL_DEBUG
      virtual void debug(const char* const where);
    #endif
    virtual float floatValue(); // Can build these for other types and combos e.g. returning bool from a float etc
    //virtual void set(const float newvalue); // Similarly - setting into types from variety of values
    //virtual void set(const bool newvalue);
    virtual String advertisement(const char * const name);
    void wireTo(String* topicPath);
    void wireTo(IO* io);
};
class IN : public IO {
  public:
    IN(char const * const sensorId, char const * const id, char const * const name, char const *color, const bool wireable);
    String advertisement(const char * const name) override;
    // TO-ADD-INxxx
    virtual float floatValue();
    virtual bool boolValue();
    virtual uint16_t uint16Value();
    virtual String StringValue();
    virtual bool convertAndSet(const String &payload);
    bool dispatchLeaf(const String &topicLeaf, const String &payload, bool isSet) override;
    virtual bool dispatchPath(const String &topicpath, const String &payload); 
    void setup(const char*);
};
class OUT : public IO {
  public:
    OUT(char const * const sensorId, char const * const id, char const * const name, char const *color, const bool wireable);
    String advertisement(const char * const name) override;
    //virtual void set(const float newvalue); // Similarly - setting into types from variety of values
    //virtual void set(const bool newvalue);
    // TO-ADD-OUTxxx
    virtual float floatValue();
    virtual bool boolValue();
    virtual uint16_t uint16Value();
    virtual String StringValue();
    virtual void sendWired();
    bool dispatchLeaf(const String &leaf, const String &payload, bool isSet) override; // Just checks control
};

// TO-ADD-INxxx
class INfloat : public IN {
  public:
    float value;
    uint8_t width;
    float min;
    float max;
    INfloat(); 
    INfloat(char const * const sensorId, char const * const id, char const * const name, float v, uint8_t width, float min, float max, char const * const color, const bool wireable);
    INfloat(const INfloat &other);
    float floatValue() override; // This is so that other subclasses e.g. INuint16 can still return a float if required
    bool boolValue() override;
    uint16_t uint16Value() override;
    String StringValue() override;

    // Copy assignment operator
    /*
    INfloat& operator=(const INfloat &other) {
      Serial.print("XXXXXX IN assignment __FILE__"); Serial.println(__LINE__); // Debugging here, because dont think this is used
      if (this != &other) {
          value = other.value;
          name = other.name;
          wireable = other.wireable
          topicTwig = other.topicTwig;
          wiredPath = other.wiredPath;
      }
      return *this;
    }
    */
    bool convertAndSet(const String &payload) override;
    void debug(const char* const where);
    String advertisement(const char * const name) override;
};
class INuint16 : public IN {
  public:
    uint16_t value;
    uint16_t min;
    uint16_t max;
    //INuint16(); 
    INuint16(char const * const sensorId, char const * const id, char const * const name, uint16_t v, uint16_t min, uint16_t max, char const * const color, const bool wireable);
    INuint16(const INuint16 &other);
    float floatValue() override; // This is so that other subclasses e.g. INuint16 can still return a float if required
    bool boolValue() override;
    uint16_t uint16Value() override;
    String StringValue() override;
    bool convertAndSet(const String &payload) override;
    void debug(const char* const where);
    String advertisement(const char * const name) override;

};
class INbool : public IN {
  public:
    bool value;
    //INbool(); 
    INbool(char const * const sensorId, char const * const id, char const * const name, bool value, char const * const color, const bool wireable);
    INbool(const INuint16 &other);
    float floatValue() override; // This is so that other subclasses e.g. INuint16 can still return a float if required
    bool boolValue() override;
    uint16_t uint16Value() override;
    String StringValue() override;
    bool convertAndSet(const String &payload) override;
    void debug(const char* const where);
    String advertisement(const char * const name) override;
};

class INcolor : public IN {
  public:
    uint8_t r;
    uint8_t g;
    uint8_t b;
    INcolor(); 
    INcolor(char const * const sensorId, char const * const id, char const * const name, uint8_t r, uint8_t g, uint8_t b, const bool wireable);
    INcolor(char const * const sensorId, char const * const id, char const * const name, char const * const color, const bool wireable);
    INcolor(const INcolor &other);
    float floatValue() override; // This is so that other subclasses e.g. INuint16 can still return a float if required
    bool boolValue() override;
    uint16_t uint16Value() override;
    String StringValue() override;
    bool convertAndSet(const String &payload) override;
    bool convertAndSet(const char* payload); // Used when setting in constructor etc
    void debug(const char* const where);
    String advertisement(const char * const name) override;
};

class INtext : public IN {
  public:
    String* value;  // dont know the type of this value  and shouldnt care
    INtext();
    INtext(const char * const sensorId, const char * const id, const char* const name, String* value, const char* const color, const bool wireable);
    INtext(const INtext &other);
    float floatValue() override; // This is so that other subclasses e.g. INuint16 can still return a float if required
    bool boolValue() override;
    uint16_t uint16Value() override;
    String StringValue() override;
    bool convertAndSet(const String &payload) override;
    bool convertAndSet(const char* payload); // Used when setting in constructor etc
    void debug(const char* const where);
    String advertisement(const char * const name) override;
};

// TO-ADD-OUTxxx
class OUTfloat : public OUT {
  public:
    float value;
    uint8_t width;
    float min;
    float max;
    OUTfloat();
    OUTfloat(char const * const sensorId, char const * const id, char const * const name, float v, uint8_t width, float min, float max, char const * const color, const bool wireable);
    OUTfloat(const OUTfloat &other);
    float floatValue() override; // This is so that other subclasses e.g. OUTuint16 can still return a float if required
    bool boolValue() override;
    uint16_t uint16Value() override;
    String StringValue() override;
    void sendWired() override;
    void set(const float newvalue); // Set and send if changed
    void debug(const char* const where);
    String advertisement(const char * const name) override;
};
class OUTbool : public OUT {
  public:
    bool value;
    OUTbool();
    OUTbool(char const * const sensorId, char const * const id, char const * const name, bool v, char const * const color, const bool wireable);
    OUTbool(const OUTbool &other);
    float floatValue() override; // This is so that other subclasses e.g. OUTuint16 can still return a float if required
    bool boolValue() override;
    uint16_t uint16Value() override;
    String StringValue() override;
    void set(const bool newvalue);
    void sendWired() override;
    void debug(const char* const where);
    String advertisement(const char * const name) override;
};
class OUTuint16 : public OUT {
  public:
    uint16_t value;
    uint16_t min;
    uint16_t max;
    OUTuint16();
    OUTuint16(char const * const sensorId, char const * const id, char const * const name, uint16_t v, uint16_t mn, uint16_t mx, char const * const color, const bool wireable);
    OUTuint16(const OUTuint16 &other);
    float floatValue() override; // This is so that other subclasses e.g. OUTuint16 can still return a float if required
    bool boolValue() override;
    uint16_t uint16Value() override;
    String StringValue() override;
    void set(const uint16_t newvalue);
    void sendWired() override;
    void debug(const char* const where);
    String advertisement(const char * const name) override;
};

#endif // BASE_H