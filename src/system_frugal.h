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
    Frugal_Group* actuators;
    Frugal_Group* sensors;
    Frugal_Group* controls;
    Frugal_Group* system;
    #ifdef SYSTEM_OTA_KEY
      System_OTA* ota;
    #endif
    #ifdef SYSTEM_TIME_WANT
      System_Time* time;
    #endif
    #ifdef SYSTEM_LORAMESHER_WANT
      System_LoraMesher* loramesher;
    #endif
    #ifdef SYSTEM_OLED_WANT
      System_OLED* oled;
    #endif
    #ifdef SYSTEM_LORA_WANT
      System_LoRa* lora;
    #endif
    System_Power_Mode* powercontroller;
    System_WiFi* wifi;
    System_MQTT* mqtt; // TODO-141 change to System_MQTT and mqtt
    System_Discovery* discovery;

    System_Frugal();
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
