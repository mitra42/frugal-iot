#ifndef SYSTEM_BASE_H
#define SYSTEM_BASE_H

//TODO which of these are needed, and if so is there a better place for them?
#include "_settings.h" // For PIN_NONE
#include <Arduino.h>
//TODO-010 Maybe needed on ESP8266
//#include <FS.h>    // ~/Documents/Arduino/hardware/esp8266com/esp8266/cores/esp8266/FS.h
#include "ESPAsyncWebServer.h" // for AsyncResponseStream"

class System_Message; // Forward declaration to avoid circular include with system_message.h

class System_Base {
  public:
    // Most of System_Base has to be public - I think (but am not sure) because while accessed from System_Frugal on a subclass of System_Base its not the class acting on itself?
    System_Base(const char * const id, const String name);
    void setupFailed(); // Called from overrides of setup() on failure.
    const char* id = nullptr; // Name of actuator, sensor or control 
    bool connected = false; 
    virtual void setup();
    virtual void dispatch(System_Message &msg);
    virtual void discover();
    void readConfigFromFS(File dir, const String* leaf);
    void writeConfigToFS(const String& topicTwig, const String& payload);
    virtual void loop();
    virtual void periodically();
    virtual void captiveLines(AsyncResponseStream* response) { };
    /* Plain-text lines for the status page, and for dumping to Serial.
     *
     * The default is nothing, because most System_Base subclasses have no IO to report. The
     * classes that do - Sensor, Actuator, Control and System_Buttons - override it to walk their
     * IOs, and System_Group to walk its members. Any class wanting to say something else about
     * itself overrides it too.
     *
     * Print* rather than the web response, so the same dump can go to Serial when a node will not
     * join WiFi and the portal cannot be reached at all.
     */
    virtual void statusLines(Print* out, bool full);
    virtual void infrequently();
    void powerUp(uint8_t pin3v3, uint8_t pin0v);
    virtual void powerUp();
    void powerDown(uint8_t pin3v3, uint8_t pin0v);
    virtual void powerDown();
    virtual void prepare() { }   // Optional - prepare before sleep (overridden in subclasses)
    virtual void recover() { }   // Optional - recover after sleep (overridden in subclasses)
    virtual System_Base* powerPins(const uint8_t power3v3, const uint8_t power0v); // Just here to allow chaining in Group
  protected: 
    /* One line "<id>/<leaf> <value>[ *]" for a module-level setting that is not an IO - the
     * things a system module handles in dispatch() and keeps in a member, such as mqtt/hostname.
     * The * means the filesystem holds it, tested against the path writeConfigToFS() uses.
     */
    void statusLine(Print* out, const char* leaf, const String& value);
    String name; // Name of actuator, sensor or control
    String leaf2path(const char* leaf);  // eg. sht/temperature or sht/temperature/max -> dev/lotus/esp123/sht/temperature ...
    String leaf2path(const String& leaf); 
    void readConfigFromFS();
}; // Class System_Base

class System_SensorActuator : public System_Base {
  public:
    System_SensorActuator(const char * const id, const String name);
    System_SensorActuator* powerPins(const uint8_t power3v3, const uint8_t power0v) override;
  protected:
    uint8_t power3v3_ = PIN_NONE;
    uint8_t power0v_ = PIN_NONE;
    virtual void powerUp();   // Optional power management - override in derived classes
    virtual void powerDown(); // Optional power management - override in derived classes
  private:
}; // Class System_SensorActuator


#endif // SYSTEM_BASE_H