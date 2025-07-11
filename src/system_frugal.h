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
#include "actuator_ledbuiltin.h"
#include "system_base.h"
#include "system_discovery.h"
#include "system_fs.h"
#ifdef ESP32
#include "system_lora.h"
#endif
#ifdef ESP32
#include "system_loramesher.h"
#endif
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
    void add(System_Base* fb);
    void dispatchTwig(const String &topicActuatorId, const String &topicLeaf, const String &payload, bool isSet); 
    void dispatchPath(const String &topicPath, const String &payload); // Only currently relevant on controls
    String advertisement();
    void frequently();
    void periodically();
    void infrequently();
};

class System_Frugal : public Frugal_Group {
  public:
    String org;
    String project;
    String description; 
    String device_name;
    Frugal_Group* actuators;
    Frugal_Group* sensors;
    Frugal_Group* controls;
    Frugal_Group* system;
    Frugal_Group* buttons;
    System_Discovery* discovery;
    #ifdef SYSTEM_LORA_WANT
      System_LoRa* lora;
    #endif
    #ifdef SYSTEM_LORAMESHER_WANT
      System_LoraMesher* loramesher;
    #endif
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
    System_Frugal(const char* org, const char* project, const char* id, const char* name);
    void configure_mqtt(const char* hostname, const char* username, const char* password);
    void configure_power(System_Power_Type t, unsigned long cycle_ms, unsigned long wake_ms);
    void startSerial(uint32_t baud, uint16_t serial_delay);
    void startSerial(); // Encapsulate setting up and starting serial
    void dispatchTwig(const String &topicTwig, const String &payload, bool isSet);
    void setup();
    void loop(); // Call this from main.cpp
    void infrequently();
    void frequently();
    void periodically();
};

extern System_Frugal frugal_iot;

 #endif // FRUGAL_IOT_H
