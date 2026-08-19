#ifndef SYSTEM_IO_H
#define SYSTEM_IO_H

#include "_settings.h" // For MQTT_RETAIN
#include "system/message.h" // For System_Message in dispatch

class IO {
  public:
    // Note that topicTwig = sensorId / id
    char const *sensorId; // Sensor this IO belongs to
    char const *id; // System readable id of this input or output
    String name; // Human readable name of this IO within the sensor, i.e. can duplicate across sensors
    const String topicTwig; // e.g. sht/temperature
    const char* color; // String passed to UX
    const char* default_color;
    // Optional unit for display only ("C", "hPa", "kOhm") - printed by Sensor::captiveLines
    // after the value. Set on the IO after constructing it; not sent to the UX (the server's
    // schema owns what the UX displays), and not part of any constructor because that would
    // mean touching every IO constructor in the library for a captive-portal nicety.
    const char* unit = nullptr;
    bool const wireable; // True if can wire this to/from others - note this flag is on the control, not on the sensor or actuator
    String wiredPath; // Topic also listening|sending to when wired
    IO();
    IO(const char * const sensorId, const char * const id, const String name, char const *color, const bool w = true);
    // Virtual only because the hierarchy is polymorphic and something deletes through an IO
    // subclass pointer: Sensor_ENS160::setup() drops aqi500 on a part that has no 0..500 index.
    // That delete is via the exact OUTuint16* type, so it was already well defined - this makes
    // it stay that way if anything ever deletes through a base pointer, and silences
    // -Wdelete-non-virtual-dtor. Nothing here owns heap memory beyond IO's own Strings, so the
    // defaulted body is all that is needed.
    virtual ~IO() = default;
    virtual void setup();
    void writeConfigToFS(const String &leaf, const String& payload);
    virtual bool dispatch(System_Message &msg);
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
  protected: // Most of IO appears to need to be public
};
class IN : public IO {
  public:
    IN(char const * const sensorId, char const * const id, const String name, char const *color, const bool wireable);
    // TO-ADD-INxxx
    virtual float floatValue();
    virtual bool boolValue();
    virtual bool convertAndSet(const String &payload);
    bool dispatch(System_Message &msg) override;
    void setup();
    void wireTo(String topicPath);
  protected: // Most of IN appears to need to be public
};
class OUT : public IO {
  public:
    OUT(char const * const sensorId, char const * const id, const String name, char const *color, const bool wireable);
    //virtual void set(const float newvalue); // Similarly - setting into types from variety of values
    //virtual void set(const bool newvalue);
    // TO-ADD-OUTxxx
    virtual float floatValue();
    virtual bool boolValue();
    virtual void sendWired(bool retain = MQTT_RETAIN, uint8_t qos = MQTT_QOS_ATLEAST1);
    bool dispatch(System_Message &msg) override;
  protected: // Most of IN appears to need to be public
};

// TO-ADD-INxxx
class INfloat : public IN {
  public:
    INfloat(); 
    INfloat(char const * const sensorId, char const * const id, const String name, float v, uint8_t width, float min, float max, char const * const color, const bool wireable);
    INfloat(char const * const sensorId, char const * const id, const String name, float v, uint8_t width, float min, float max, float default_min, float default_max, char const * const color, const bool wireable);
    INfloat(const INfloat &other);
    float floatValue() override; // This is so that other subclasses e.g. INuint16 can still return a float if required
    uint8_t width; // Cant be protected because used in e.g. control_oled_ht.cpp 
    virtual String StringValue();
    void discover() override;
    bool dispatch(System_Message &msg) override;
  protected:
    float value;
    float min;
    float max;
    float default_min;
    float default_max;
    bool boolValue() override;
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
};
class INuint16 : public IN {
  public:
    uint16_t value;
    uint16_t min;
    uint16_t max;
    //INuint16();
    INuint16(char const * const sensorId, char const * const id, const String name, uint16_t v, uint16_t min, uint16_t max, char const * const color, const bool wireable);
    INuint16(char const * const sensorId, char const * const id, const String name, uint16_t v, uint16_t min, uint16_t max, uint16_t default_min, uint16_t default_max, char const * const color, const bool wireable);
    INuint16(const INuint16 &other);
    bool dispatch(System_Message &msg) override;
    void discover() override;
  protected:
    uint16_t default_min;
    uint16_t default_max;
    float floatValue() override; // This is so that other subclasses e.g. INuint16 can still return a float if required
    bool boolValue() override;
    virtual String StringValue();
    bool convertAndSet(const String &payload) override;
    void debug(const char* const where);
};
class INbool : public IN {
  public:
    //INbool(); 
    INbool(char const * const sensorId, char const * const id, const String name, bool value, char const * const color, const bool wireable);
    INbool(const INuint16 &other);
    bool value;
  protected:
    float floatValue() override; // This is so that other subclasses e.g. INuint16 can still return a float if required
    bool boolValue() override;
    virtual String StringValue();
    bool convertAndSet(const String &payload) override;
    void debug(const char* const where);
    // void discover() override; // Use IN::discover
};

class INcolor : public IN {
  public:  
    INcolor(); 
    INcolor(char const * const sensorId, char const * const id, const String name, uint8_t r, uint8_t g, uint8_t b, const bool wireable);
    INcolor(char const * const sensorId, char const * const id, const String name, char const * const color, const bool wireable);
  
    INcolor(const INcolor &other);
    uint8_t r;
    uint8_t g;
    uint8_t b;
  protected:
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
    INtext();
    INtext(const char * const sensorId, const char * const id, const String name, String value, char const * const color, const bool wireable);
    INtext(const INtext &other);
    String value;  // dont know the type of this value  and shouldnt care
  protected:
    float floatValue() override; // This is so that other subclasses e.g. INuint16 can still return a float if required
    bool boolValue() override;
    virtual String StringValue();
    bool convertAndSet(const String &payload) override;
    bool convertAndSet(const char* payload); // Used when setting in constructor etc
    void debug(const char* const where);
    //void discover() override;
};

// TO-ADD-OUTxxx
class OUTfloat : public OUT {
  public:
    uint8_t width; // Not protected because used in e.g. captive or OLED output
    float min; // Not protected cos used in e.g. captive lines 
    float max; // Not protected cos used in e.g. captive lines 
    float default_min;
    float default_max;
    OUTfloat();
    OUTfloat(char const * const sensorId, char const * const id, const String name, float v, uint8_t width, float min, float max, char const * const color, const bool wireable);
    OUTfloat(char const * const sensorId, char const * const id, const String name, float v, uint8_t width, float min, float max, float default_min, float default_max, char const * const color, const bool wireable);
    OUTfloat(const OUTfloat &other);
    void set(const float newvalue); // Set and send if changed
    bool dispatch(System_Message &msg) override;
    void discover() override;
    float floatValue() override; // This is so that other subclasses e.g. OUTuint16 can still return a float if required
    bool boolValue() override;
    virtual String StringValue();
  protected:
    float value;
    void debug(const char* const where);
};
class OUTbool : public OUT {
  public:
    bool value;
    OUTbool();
    OUTbool(char const * const sensorId, char const * const id, const String name, bool v, char const * const color, const bool wireable);
    OUTbool(const OUTbool &other);
    void set(const bool newvalue);
    float floatValue() override; // This is so that other subclasses e.g. OUTuint16 can still return a float if required
    bool boolValue() override;
    virtual String StringValue();
  protected:
    bool dispatch(System_Message &msg) override;
    void send() override;
    void debug(const char* const where);
    // void discover() override; // Use OUT::discover
};
class OUTuint16 : public OUT {
  public:
    uint16_t value;
    uint16_t min;
    uint16_t max;
    uint16_t default_min;
    uint16_t default_max;
    OUTuint16();
    OUTuint16(char const * const sensorId, char const * const id, const String name, uint16_t v, uint16_t mn, uint16_t mx, char const * const color, const bool wireable);
    OUTuint16(const OUTuint16 &other);
    void set(const uint16_t newvalue);
    void discover() override;
    float floatValue() override; // This is so that other subclasses e.g. OUTuint16 can still return a float if required
    bool boolValue() override;
    virtual String StringValue();
  protected:
    bool dispatch(System_Message &msg) override;
    void debug(const char* const where);
};
class OUTtext : public OUT {
  public:
    String value;
    OUTtext(char const * const sensorId, char const * const id, const String name, const String v, char const * const color="#000000", const bool wireable=false);
    void set(const String newvalue);
    //bool dispatch(const String &leaf, const String &p, bool isSet) override;
    //void discover() override;
    //float floatValue() override; // This is so that other subclasses e.g. OUTuint16 can still return a float if required
    //bool boolValue() override;
    virtual String StringValue();
  protected:
    void debug(const char* const where); // Uncommented: implementation exists in system_base.cpp but declaration was commented out
};

#endif // SYSTEM_IO_H