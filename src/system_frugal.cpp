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
 *  Move material from main.cpp::setup to System_Frugal::setup
 */


#include "_settings.h" // Note - ideally shouldnt be dependent on anything here, or at least not in _local.h
#include "system_frugal.h"
#include "misc.h"

Frugal_Group::Frugal_Group(const char * const id, const char * const name)
: System_Base(id, name)
{}

void Frugal_Group::add(System_Base* fb) {
  group.push_back(fb);
}


// TODO-141 move most of main.cpp::setup to here, all non-app stuff
void Frugal_Group::setup() {
  for (System_Base* fb: group) {
    #ifdef SYSTEM_FRUGAL_DEBUG
      Serial.print(fb->id); Serial.print(F(" "));
    #endif
    fb->setup();
  }
}

void Frugal_Group::dispatchTwig(const String &id, const String &topicLeaf, const String &payload, bool isSet) {
  for (System_Base* fb: group) {
    fb->dispatchTwig(id, topicLeaf, payload, isSet);
  }
};

void System_Frugal::dispatchTwig(const String &topicTwig, const String &payload, bool isSet) {
  // topic Twig  <actuatorId>/<ioID> or  <actuatorId>/set/<ioID> or <actuatorId>/set/<ioID>/<config>
  uint8_t slashPos = topicTwig.indexOf('/'); // Find the position of the slash
  if (slashPos != -1) {
    String id = topicTwig.substring(0, slashPos);       // Extract the part before the slash
    String topicLeaf = topicTwig.substring(slashPos + 1);      // Extract the part after the slash
    Frugal_Group::dispatchTwig(id, topicLeaf, payload, isSet);
  } else {
    Serial.println("No slash found in topic: " + topicTwig);
  }
}
void Frugal_Group::dispatchPath(const String &topicPath, const String &payload) {
  for (System_Base* fb: group) {
    fb->dispatchPath(topicPath, payload);
  }
}

String Frugal_Group::advertisement() {
  String ad = String();
  for (System_Base* fb: group) {
    ad += (fb->advertisement());
  }
  return ad;
}

void Frugal_Group::frequently() {
  for (System_Base* fb: group) {
    fb->frequently();
  }
}
void Frugal_Group::periodically() {
  for (System_Base* fb: group) {
    fb->periodically();
  }
}
void Frugal_Group::infrequently() {
  for (System_Base* fb: group) {
    fb->infrequently();
  }
}

System_Frugal::System_Frugal()
: Frugal_Group("frugal_iot", "System_Frugal"),
  actuators(new Frugal_Group("actuators", "Actuators")),
  sensors(new Frugal_Group("sensors", "Sensors")),
  controls(new Frugal_Group("controls", "Controls")),
  system(new Frugal_Group("system", "System")),
  time(nullptr), // time is optional and setup by main.cpp if needed
  #ifdef SYSTEM_OTA_KEY
    ota(new System_OTA()),
  #endif
  #ifdef SYSTEM_OLED_WANT // Set in _settings.h on applicable boards or can be added by main.cpp
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
  #ifdef LED_BUILTIN // defined by board or _settings.h
    actuators->add(new Actuator_Ledbuiltin(LED_BUILTIN)); // Default LED builtin actuator at default brightness and white
  #endif
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
  #ifdef SYSTEM_OTA_KEY
    system->add(ota);
  #endif
  #ifdef SYSTEM_LORA_WANT
    system->add(lora);
  #endif // SYSTEM_LORA_WANT

  add(system);
}

void System_Frugal::setup() {
  #ifdef SYSTEM_FRUGAL_DEBUG
    Serial.print("Setup: ");
  #endif
  Frugal_Group::setup(); // includes WiFi
  if (time) { // If time has been constructed
    time->setup_after_wifi();
  }
  mqtt->setup_after_wifi();
  discovery->setup_after_mqtt();  // System_Discovery
  #ifdef SYSTEM_OTA_KEY
    ota->setup_after_discovery();
  #endif
  #ifdef SYSTEM_FRUGAL_DEBUG
     Serial.println();
  #endif
}
void System_Frugal::infrequently() {
  Frugal_Group::infrequently();
  #ifdef LOCAL_DEV_WANT
    // TODO-141 this will go back into the new main.cpp in some form
    localDev::infrequently();
  #endif
}

// These are things done one time per period - where a period is the time set in SYSTEM_POWER_MS
void System_Frugal::periodically() {
  Frugal_Group::periodically();
  #ifdef LOCAL_DEV_WANT
    // TODO-141 this will go back into the new main.cpp in some form
    localDev::periodically();
  #endif
}

// This is stuff done multiple times per period
void System_Frugal::frequently() {
  Frugal_Group::frequently();
  frugal_iot.mqtt->frequently(); // 
  #ifdef LOCAL_DEV_WANT //TODO-141 move to new app specific main.cpp
    localDev::frequently();
  #endif
  // TODO-23 will want something here for buttons as well
}

// Main loop() - call this from main.cpp
void System_Frugal::loop() {
  static bool donePeriodic = false;
  if (!donePeriodic) {
    periodically();  // Do things run once per cycle
    infrequently();  // Do things that keep their own track of time
    donePeriodic = true;
  }
  frequently(); // Do things like MQTT which run frequently with their own clock
  if (powercontroller->maybeSleep()) { // Note this returns true if sleep, OR if period for POWER_MODE_LOOP
    donePeriodic = false; // reset after sleep (note deep sleep comes in at top again)
  }
}

#if !defined(SERIAL_DELAY)
  #define SERIAL_DELAY 5000
#endif

void System_Frugal::startSerial() {
  // Encapsulate setting up and starting serial
  #ifdef ANY_DEBUG
    Serial.begin(SERIAL_BAUD);
    while (!Serial) { 
      ; // wait for serial port to connect. Needed for Arduino Leonardo only
    }
    delay(SERIAL_DELAY); // If dont do this on D1 Mini and Arduino IDE then miss next debugging
    Serial.println(F("FrugalIoT Starting"));
  #endif // ANY_DEBUG  
}
