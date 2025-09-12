/* Frugal-Iot main controller class 
 * 
 * One class to rule them all .... 
 * You should be able to get to any data structure from here.
 * 
 * DRAFT: This is a first-cut, it will be incrementally added to while (hopefully) maintaining 
 * working code, though potentially at the risk of inconsistency 
 * i.e. some things will be handled the OLD way, and some the new - through here.
 * 
 * See https://github.com/mitra42/frugal-iot/issues/141 for discussion and task list
 * 
 */

 #ifndef FRUGAL_IOT_H
 #define FRUGAL_IOT_H

#include <vector>
// Non Frugal IoT headers
#include "ESPAsyncWebServer.h" // for AsyncResponseStream"
// Frugal-iot headers
#include "_settings.h" 
#include "actuator_ledbuiltin.h"
#include "system_base.h"
#include "system_captive.h"
#include "system_discovery.h"
#include "system_fs.h"
#include "system_language.h"
#ifdef ESP32
#include "system_loramesher.h"
#endif
#include "system_message.h"
#include "system_mqtt.h"
#include "system_oled.h"
#include "system_ota.h"
#include "system_power.h"
#include "system_time.h"
#include "system_watchdog.h"
#include "system_wifi.h"

class Frugal_Group : public System_Base {
  public:
    std::vector<System_Base*> group;
    Frugal_Group(const char * const id, const char * const name);
    void setup();
    void setup_after_wifi();
    void setup_after_mqtt();
    void add(System_Base* fb);
    void dispatchTwig(const String &topicActuatorId, const String &topicLeaf, const String &payload, bool isSet); 
    void dispatchPath(const String &topicPath, const String &payload) override; // Only currently relevant on controls
    void discover() override;
    void loop() override;
    void periodically() override;
    void infrequently() override;
    void captiveLines(AsyncResponseStream* response) override; 
};

class System_Frugal : public Frugal_Group {
  public:
    // Configuration strings 
    String org;
    String project;
    String description; 
    const String nodeid; // Unique id - starts esp32- or esp8266-
    // Pointers to other Frugal_Base objects or groups of objects
    Frugal_Group* actuators;
    Frugal_Group* sensors;
    Frugal_Group* controls;
    Frugal_Group* system;
    Frugal_Group* buttons;
    System_Captive* captive;
    System_Discovery* discovery;
    #ifdef SYSTEM_LORAMESHER_WANT // This is automatically defined on LoRa compatable boardss
      System_LoraMesher* loramesher; // Will be nullptr if no loramesher
    #endif
    System_Messages* messages;
    System_MQTT* mqtt;
    #ifdef SYSTEM_OLED_WANT
      System_OLED* oled;
    #endif
    #ifdef SYSTEM_OTA_KEY
      System_OTA* ota;
    #endif
    System_Power_Mode* powercontroller;
    System_LittleFS* fs_LittleFS; 
    System_Time* time; // Optional - may be nullptr if not set up
    System_WiFi* wifi;
    // Operational
    bool timeForPeriodic = true;
    System_Frugal(const char* org, const char* project, const char* id, const char* name);
    void configure_mqtt(const char* hostname, const char* username, const char* password);
    void configure_power(System_Power_Type t, unsigned long cycle_ms, unsigned long wake_ms);
    void startSerial(uint32_t baud, uint16_t serial_delay);
    void startSerial(); // Encapsulate setting up and starting serial
    void dispatchTwig(const String &topicSensorId, const String &topicLeaf, const String &payload, bool isSet) override; // this is for local messages for ths obj
    void dispatchTwig(const String &topicTwig, const String &payload, bool isSet); // this is the looping one
    void pre_setup(); // Setup done before Messages tries to access "project"
    void setup() override;
    void setup_after_wifi();
    void loop() override; // Call this from main.cpp
    void infrequently() override;
    void periodically() override;
    void captiveLines(AsyncResponseStream* response) override; 
    bool canOTA();
    bool canMQTT();
    void discover() override;
};

extern System_Frugal frugal_iot;

 #endif // FRUGAL_IOT_H
