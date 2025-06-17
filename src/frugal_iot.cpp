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
 * QUICK TODOs - remove as done
 * 
 * Incremental tasks
 *  Move material from main.cpp::setup to Frugal_IoT::setup
 */


#include "_settings.h" // Note - ideally shouldnt be dependent on anything here, or at least not in _local.h
#include "frugal_iot.h"
#include "misc.h"

Frugal_Group::Frugal_Group(const char * const id, const char * const name)
: Frugal_Base(id, name)
{}

void Frugal_Group::add(Frugal_Base* fb) {
  group.push_back(fb);
}


// TODO-141 move most of main.cpp::setup to here, all non-app stuff
void Frugal_Group::setup() {
  for (Frugal_Base* fb: group) {
    Serial.print(fb->id); Serial.print(F(" "));
    fb->setup();
  }
}

void Frugal_Group::dispatchTwig(const String &topicTwig, const String &payload, bool isSet) {
  // topic Twig  <actuatorId>/<ioID> or  <actuatorId>/set/<ioID> or <actuatorId>/set/<ioID>/<config>
  // This came from Actuators - not checked on Sensors or Control yet
  uint8_t slashPos = topicTwig.indexOf('/'); // Find the position of the slash
  if (slashPos != -1) {
    String id = topicTwig.substring(0, slashPos);       // Extract the part before the slash
    String topicLeaf = topicTwig.substring(slashPos + 1);      // Extract the part after the slash
    for (Frugal_Base* fb: group) {
      fb->dispatchTwig(id, topicLeaf, payload, isSet);
    }
  } else {
    Serial.println("No slash found in topic: " + topicTwig);
  }
}
void Frugal_Group::dispatchPath(const String &topicPath, const String &payload) {
  for (Frugal_Base* fb: group) {
    fb->dispatchPath(topicPath, payload);
  }
}

String Frugal_Group::advertisement() {
  String ad = String();
  for (Frugal_Base* fb: group) {
    ad += (fb->advertisement());
  }
  return ad;
}

void Frugal_Group::frequently() {
  for (Frugal_Base* fb: group) {
    fb->frequently();
  }
}
void Frugal_Group::periodically() {
  for (Frugal_Base* fb: group) {
    fb->periodically();
  }
}
void Frugal_Group::infrequently() {
  for (Frugal_Base* fb: group) {
    fb->infrequently();
  }
}

Frugal_IoT::Frugal_IoT()
: Frugal_Group("frugal_iot", "Frugal_IoT"),
  actuators(new Frugal_Group("actuators", "Actuators")),
  sensors(new Frugal_Group("sensors", "Sensors")),
  controls(new Frugal_Group("controls", "Controls")),
  system(new Frugal_Group("system", "System")),
  #ifdef SYSTEM_OTA_WANT
    ota(new System_OTA()),
  #endif
  #ifdef SYSTEM_TIME_WANT
    time(new System_Time()),
  #endif
  #ifdef SYSTEM_LORAMESHER_WANT
    loramesher(new System_LoraMesher()),
  #endif
  #ifdef SYSTEM_OLED_WANT
    // TODO-141 move into frugal_iot. 
    oled(new System_OLED()),
  #endif // SYSTEM_OLED_WANT
  #ifdef SYSTEM_LORA_WANT
    lora(new System_LoRa()),
  #endif
  // TO-ADD-POWERMODE
  #ifdef SYSTEM_POWER_MODE_LOOP
    powercontroller(new System_Power_Mode_Loop(SYSTEM_POWER_MS, SYSTEM_POWER_WAKE_MS)),
  #elif SYSTEM_POWER_MODE_LIGHT
    powercontroller(new System_Power_Mode_Light(SYSTEM_POWER_MS, SYSTEM_POWER_WAKE_MS)),
  #elif SYSTEM_POWER_MODE_LIGHTWIFI
    powercontroller(new System_Power_Mode_LightWifi(SYSTEM_POWER_MS, SYSTEM_POWER_WAKE_MS)),
  #elif SYSTEM_POWER_MODE_DEEP
    powercontroller(new System_Power_Mode_Deep(SYSTEM_POWER_MS, SYSTEM_POWER_WAKE_MS)),
  #elif SYSTEM_POWER_MODE_AUTO
    powercontroller(new System_Power_Mode_Auto(SYSTEM_POWER_MS, SYSTEM_POWER_WAKE_MS)),
  #endif
  wifi(new System_WiFi()),
  mqtt(new System_MQTT()),
  discovery(new System_Discovery())
{
  add(actuators);
  add(sensors);
  add(controls);
  system->add(wifi);
  system->add(mqtt);
  system->add(discovery);
  system->add(powercontroller);
  system->add(new System_Watchdog());
  #ifdef SYSTEM_OLED_WANT
    system->add(oled);
  #endif
  #ifdef SYSTEM_OTA_WANT
    system->add(ota);
  #endif
  #ifdef SYSTEM_LORAMESHER_WANT
    system->add(loramesher);
  #endif
  #ifdef SYSTEM_TIME_WANT
    system->add(time);
  #endif
  #ifdef SYSTEM_LORA_WANT
    system->add(lora);
  #endif // SYSTEM_LORA_WANT

  add(system);
}

void Frugal_IoT::setup() {
  Frugal_Group::setup(); // includes WiFi
  #ifdef SYSTEM_OTA_WANT
    ota->setup_after_wifi();
  #endif
  #ifdef SYSTEM_TIME_WANT
    time->setup_after_wifi();
  #endif
  mqtt->setup_after_wifi();
  discovery->setup_after_mqtt();  // System_Discovery
  #ifdef LOCAL_DEV_WANT
    localDev::setup();
  #endif
}
void Frugal_IoT::infrequently() {
  Frugal_Group::infrequently();
  #ifdef LOCAL_DEV_WANT
    // TODO-141 this will go back into the new main.cpp in some form
    localDev::infrequently();
  #endif
}

// These are things done one time per period - where a period is the time set in SYSTEM_POWER_MS
void Frugal_IoT::periodically() {
  Frugal_Group::periodically();
  #ifdef LOCAL_DEV_WANT
    // TODO-141 this will go back into the new main.cpp in some form
    localDev::periodically();
  #endif
}

// This is stuff done multiple times per period
// TODO-141 move into frugal_iot. 
void Frugal_IoT::frequently() {
  Frugal_Group::frequently();
  frugal_iot.mqtt->frequently(); // 
  #ifdef LOCAL_DEV_WANT //TODO-141 move to new app specific main.cpp
    localDev::frequently();
  #endif
  // TODO-23 will want something here for buttons as well
}


