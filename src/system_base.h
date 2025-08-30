#ifndef BASE_H
#define BASE_H

#include <Arduino.h>
#include <FS.h>    // ~/Documents/Arduino/hardware/esp8266com/esp8266/cores/esp8266/FS.h
#include "ESPAsyncWebServer.h" // for AsyncResponseStream"

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
    String name; // Name of actuator, sensor or control
    System_Base(const char * const id, const String name);
    virtual void setup();
    virtual void dispatchTwig(const String &topicActuatorId, const String &topicLeaf, const String &payload, bool isSet);
    virtual void dispatchPath(const String &topicPath, const String &payload);
    String leaf2path(const char* leaf); 
    virtual void discover();
    void readConfigFromFS();
    void readConfigFromFS(File dir, const String* leaf);
    void writeConfigToFS(const String& topicTwig, const String& payload);
    virtual void loop();
    virtual void periodically();
    virtual void infrequently();
    virtual void captiveLines(AsyncResponseStream* response) { };
}; // Class FrugalBase

class IO {
  public:
    // Note that topicTwig = sensorId / id
    char const *sensorId; // Sensor this IO belongs to
    char const *id; // System readable id
    String name; // Human readable name of this IO within the sensor, i.e. can duplicate across sensors
    const String topicTwig;
    const char* color; // String passed to UX
    bool const wireable; // True if can wire this to/from others - note this flag is on the control, not on the sensor or actuator
    String wiredPath; // Topic also listening|sending to when wired
    String* wiredPathXXX; // Topic also listening|sending to when wired
    IO();
    IO(const char * const sensorId, const char * const id, const String name, char const *color, const bool w = true);
    virtual void setup();
    void writeConfigToFS(const String &leaf, const String& payload);
    virtual bool dispatchLeaf(const String &topicLeaf, const String &payload, bool isSet); // Just checks control
    virtual bool dispatchPath(const String &topicPath, const String &payload);
    virtual String StringValue();
    virtual void send();
    #ifdef CONTROL_DEBUG
      virtual void debug(const char* const where);
    #endif
    virtual float floatValue(); // Can build these for other types and combos e.g. returning bool from a float etc
    //virtual void set(const float newvalue); // Similarly - setting into types from variety of values
    //virtual void set(const bool newvalue);
    virtual void discover();
    void wireTo(String topicPath);
    void wireTo(IO* io);
    String path();
};
class IN : public IO {
  public:
    IN(char const * const sensorId, char const * const id, const String name, char const *color, const bool wireable);
    // TO-ADD-INxxx
    virtual float floatValue();
    virtual bool boolValue();
    virtual bool convertAndSet(const String &payload);
    bool dispatchLeaf(const String &topicLeaf, const String &payload, bool isSet) override;
    virtual bool dispatchPath(const String &topicpath, const String &payload); 
    void setup();
};
class OUT : public IO {
  public:
    OUT(char const * const sensorId, char const * const id, const String name, char const *color, const bool wireable);
    //virtual void set(const float newvalue); // Similarly - setting into types from variety of values
    //virtual void set(const bool newvalue);
    // TO-ADD-OUTxxx
    virtual float floatValue();
    virtual bool boolValue();
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
    INfloat(char const * const sensorId, char const * const id, const String name, float v, uint8_t width, float min, float max, char const * const color, const bool wireable);
    INfloat(const INfloat &other);
    float floatValue() override; // This is so that other subclasses e.g. INuint16 can still return a float if required
    bool boolValue() override;
    virtual String StringValue();
    bool dispatchLeaf(const String &leaf, const String &p, bool isSet);
    // Copy assignment operator
    /*
    INfloat& operator=(const INfloat &other) {
      Serial.print(F("XXXXXX IN assignment __FILE__")); Serial.println(__LINE__); // Debugging here, because dont think this is used
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
    void discover() override;
};
class INuint16 : public IN {
  public:
    uint16_t value;
    uint16_t min;
    uint16_t max;
    //INuint16(); 
    INuint16(char const * const sensorId, char const * const id, const String name, uint16_t v, uint16_t min, uint16_t max, char const * const color, const bool wireable);
    INuint16(const INuint16 &other);
    float floatValue() override; // This is so that other subclasses e.g. INuint16 can still return a float if required
    bool boolValue() override;
    virtual String StringValue();
    bool convertAndSet(const String &payload) override;
    void debug(const char* const where);
    void discover() override;
    bool dispatchLeaf(const String &leaf, const String &p, bool isSet);
};
class INbool : public IN {
  public:
    bool value;
    //INbool(); 
    INbool(char const * const sensorId, char const * const id, const String name, bool value, char const * const color, const bool wireable);
    INbool(const INuint16 &other);
    float floatValue() override; // This is so that other subclasses e.g. INuint16 can still return a float if required
    bool boolValue() override;
    virtual String StringValue();
    bool convertAndSet(const String &payload) override;
    void debug(const char* const where);
    // void discover() override; // Use IN::discover
};

class INcolor : public IN {
  public:
    uint8_t r;
    uint8_t g;
    uint8_t b;
    INcolor(); 
    INcolor(char const * const sensorId, char const * const id, const String name, uint8_t r, uint8_t g, uint8_t b, const bool wireable);
    INcolor(char const * const sensorId, char const * const id, const String name, char const * const color, const bool wireable);
    INcolor(const INcolor &other);
    float floatValue() override; // This is so that other subclasses e.g. INuint16 can still return a float if required
    bool boolValue() override;
    virtual String StringValue();
    bool convertAndSet(const String &payload) override;
    bool convertAndSet(const char* payload); // Used when setting in constructor etc
    void debug(const char* const where);
    //void discover() override; // Use base class
};

class INtext : public IN {
  public:
    String value;  // dont know the type of this value  and shouldnt care
    INtext();
    INtext(const char * const sensorId, const char * const id, const String name, String value, char const * const color, const bool wireable);
    INtext(const INtext &other);
    float floatValue() override; // This is so that other subclasses e.g. INuint16 can still return a float if required
    bool boolValue() override;
    virtual String StringValue();
    bool convertAndSet(const String &payload) override;
    bool convertAndSet(const char* payload); // Used when setting in constructor etc
    void debug(const char* const where);
    void discover() override;
};

// TO-ADD-OUTxxx
class OUTfloat : public OUT {
  public:
    float value;
    uint8_t width;
    float min;
    float max;
    OUTfloat();
    OUTfloat(char const * const sensorId, char const * const id, const String name, float v, uint8_t width, float min, float max, char const * const color, const bool wireable);
    OUTfloat(const OUTfloat &other);
    float floatValue() override; // This is so that other subclasses e.g. OUTuint16 can still return a float if required
    bool boolValue() override;
    virtual String StringValue();
    void set(const float newvalue); // Set and send if changed
    void debug(const char* const where);
    void discover() override;
    bool dispatchLeaf(const String &leaf, const String &p, bool isSet);
};
class OUTbool : public OUT {
  public:
    bool value;
    OUTbool();
    OUTbool(char const * const sensorId, char const * const id, const String name, bool v, char const * const color, const bool wireable);
    OUTbool(const OUTbool &other);
    float floatValue() override; // This is so that other subclasses e.g. OUTuint16 can still return a float if required
    bool boolValue() override;
    virtual String StringValue();
    void set(const bool newvalue);
    void send() override;
    void debug(const char* const where);
    // void discover() override; // Use OUT::discover
};
class OUTuint16 : public OUT {
  public:
    uint16_t value;
    uint16_t min;
    uint16_t max;
    OUTuint16();
    OUTuint16(char const * const sensorId, char const * const id, const String name, uint16_t v, uint16_t mn, uint16_t mx, char const * const color, const bool wireable);
    OUTuint16(const OUTuint16 &other);
    float floatValue() override; // This is so that other subclasses e.g. OUTuint16 can still return a float if required
    bool boolValue() override;
    virtual String StringValue();
    void set(const uint16_t newvalue);
    void debug(const char* const where);
    void discover() override;
    bool dispatchLeaf(const String &leaf, const String &p, bool isSet);
};

#endif // BASE_H