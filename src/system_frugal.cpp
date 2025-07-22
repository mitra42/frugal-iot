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

 // Defines because cannot pass parameters to constructor
#ifndef SYSTEM_FRUGAL_PROJECT
  #define SYSTEM_FRUGAL_PROJECT "developers"
#endif
#ifndef SYSTEM_MQTT_HOST
  #define SYSTEM_MQTT_HOST "frugaliot.naturalinnovation.org"
#endif
#ifndef SYSTEM_MQTT_PASSWORD
  #define SYSTEM_MQTT_PASSWORD "public"
#endif    


#include "_settings.h" // Note - ideally shouldnt be dependent on anything here, or at least not in _local.h
#include "system_frugal.h"
#include "misc.h"

Frugal_Group::Frugal_Group(const char * const id, const char * const name)
: System_Base(id, name)
{}

void Frugal_Group::add(System_Base* fb) {
  group.push_back(fb);
}


void Frugal_Group::setup() {
  for (System_Base* fb: group) {
    #ifdef SYSTEM_FRUGAL_DEBUG
      Serial.println(fb->id);
    #endif
    fb->setup();
  }
}

void Frugal_Group::dispatchTwig(const String &topicSensorId, const String &topicLeaf, const String &payload, bool isSet) {
  for (System_Base* fb: group) {
    fb->dispatchTwig(topicSensorId, topicLeaf, payload, isSet);
  }
};

// Handle messages at top level - check for own, and if not loop through all other modules
// e.g. topicSensorId: "sht30"  topicTwig: "temperature" or "temperature/max"  payload="23.0" 
void System_Frugal::dispatchTwig(const String &topicSensorId, const String &topicTwig, const String &payload, bool isSet) {
  if (isSet && (topicSensorId == id)) {
    if (topicTwig == "project") {
      project = payload;
    } else if (topicTwig == "device_name") {
      device_name = payload;
    } else if (topicTwig == "description") {
      description = payload;
    }
    writeConfigToFS(topicTwig, payload);
  } else { // No point in passing on our own id for the loop
    Frugal_Group::dispatchTwig(topicSensorId, topicTwig, payload, isSet);
  }
}
void System_Frugal::captiveLines(AsyncResponseStream* response) {
  captive->string(response, id, "project", project, "Project", 2, 15);
  captive->string(response, id, "device_name", device_name, "Device Name", 3, 15);
  captive->string(response, id, "description", description, "Description", 3, 40);
  Frugal_Group::captiveLines(response);
}

void System_Frugal::dispatchTwig(const String &topicTwig, const String &payload, bool isSet) {
  // topic Twig  <actuatorId>/<ioID> or  <actuatorId>/<ioID> or <actuatorId>/<ioID>/<config>
  int8_t slashPos = topicTwig.indexOf('/'); // Find the position of the slash
  if (slashPos != -1) {
    String id = topicTwig.substring(0, slashPos);       // Extract the part before the slash
    String topicLeaf = topicTwig.substring(slashPos + 1);      // Extract the part after the slash
    dispatchTwig(id, topicLeaf, payload, isSet);
  } else {
    Serial.print(F("No slash found in topic: ")); Serial.println(topicTwig);
  }
}

String Frugal_Group::advertisement() {
  String ad = String();
  for (System_Base* fb: group) {
    ad += (fb->advertisement());
  }
  return ad;
}

// These just loop over the members of the group 
void Frugal_Group::loop()         { for (System_Base* fb: group) { fb->loop(); } }
void Frugal_Group::periodically() { for (System_Base* fb: group) { fb->periodically();} }
void Frugal_Group::infrequently() { for (System_Base* fb: group) { fb->infrequently(); } }
void Frugal_Group::captiveLines(AsyncResponseStream* response) 
  { for (System_Base* fb: group) { fb->captiveLines(response); } }
void Frugal_Group::dispatchPath(const String &topicPath, const String &payload) 
  { for (System_Base* fb: group) { fb->dispatchPath(topicPath, payload); } }

// Note this constructor is running BEFORE Serial is enabledd
System_Frugal::System_Frugal(const char* org, const char* project, const char* device_name, const char* description)
: Frugal_Group("frugal_iot", "System_Frugal"),
  org(org),
  project(project),
  description(description),
  device_name(device_name),
  actuators(new Frugal_Group("actuators", "Actuators")),
  sensors(new Frugal_Group("sensors", "Sensors")),
  controls(new Frugal_Group("controls", "Controls")),
  system(new Frugal_Group("system", "System")),
  buttons(new Frugal_Group("buttons", "Buttons")),
  captive(new System_Captive()),
  discovery(new System_Discovery()), 
  #ifdef SYSTEM_LORA_WANT
    lora(new System_LoRa()),
  #endif
  // mqtt is added in main.cp > configure_mqtt(host,user,password)
  #ifdef SYSTEM_OLED_WANT // Set in _settings.h on applicable boards or can be added by main.cpp
    oled(new System_OLED()),
  #endif // SYSTEM_OLED_WANT
  #ifdef SYSTEM_OTA_KEY
    ota(new System_OTA()),
  #endif
  fs_LittleFS(new System_LittleFS()),
  time(nullptr), // time is optional and setup by main.cpp if needed
  wifi(new System_WiFi())
{
  #ifdef LED_BUILTIN // defined by board or _settings.h
    actuators->add(new Actuator_Ledbuiltin(LED_BUILTIN)); // Default LED builtin actuator at default brightness and white
  #endif
  add(actuators);
  add(sensors);
  add(controls);
  // add(buttons); // optimizing by not adding this - its only needed for looping for infrequently()
  system->add(fs_LittleFS);
  system->add(wifi);
  system->add(discovery);
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
  system->add(captive);
  add(system);
  // These things should really be in setup() but we want them to run before the rest of the main.cpp setup()
  // Note this is running BEFORE Serial is enabled
}

void System_Frugal::configure_mqtt(const char* hostname, const char* username, const char* password) {
  system->add(mqtt = new System_MQTT(hostname, username, password));  
}
void System_Frugal::configure_power(System_Power_Type t, unsigned long cycle_ms, unsigned long wake_ms) {
  system->add(powercontroller = System_Power_Mode::create(t, cycle_ms, wake_ms));  
}
void System_Frugal::setup() {
  // By the time this is run, mqtt should have been added, and serial started in main.cpp
  #ifdef SYSTEM_FRUGAL_DEBUG
    Serial.print("Setup: ");
  #endif
  readConfigFromFS(); // Reads config (project, device_name) and passes to our dispatchTwig
  #if defined(SYSTEM_LORAMESHER_SENDER_TEST) || defined(SYSTEM_LORAMESHER_RECEIVER_TEST)
    //esp_log_level_set("*", ESP_LOG_VERBOSE); // To get lots of logging from LoraMesher
    esp_log_level_set(LM_TAG, ESP_LOG_VERBOSE); // To get lots of logging from LoraMesher
  #endif
  // Use the unique id of the ESP32 or ESP8266
  #ifdef ESP32
    nodeid =  String(F("esp32-")) + (Sprintf("%06" PRIx64, ESP.getEfuseMac() >> 24));
  #elif defined(ESP8266)
    nodeid = String(F("esp8266-")) + (Sprintf("%06" PRIx32, ESP.getChipId()));
  #else
    #error Only defined for ESP32 and ESP8266
  #endif



// TODO-152 next is blocking if WiFi fails - fix that
Frugal_Group::setup(); // includes WiFi
  #ifdef SYSTEM_OTA_KEY
    ota->setup_after_mqtt_setup(); // Sends advertisement over MQTT
  #endif
  #ifdef SYSTEM_FRUGAL_DEBUG
     Serial.println();
  #endif
}
// This is called from the loop() when wifi state machine gets to connected
void System_Frugal::setup_after_wifi() {
  static bool setup_after_wifi_done = false;
  if (!setup_after_wifi_done) {
    setup_after_wifi_done = true; 
    if (time) { // If time has been constructed
      time->setup_after_wifi();
    }
    mqtt->setup_after_wifi();
  }
}
void System_Frugal::infrequently() {
  Frugal_Group::infrequently();
  buttons->infrequently(); // Not in the main group of groups as infrequently() is only thing called.
}

// These are things done one time per period - where a period is the time set in powercontroller->cycle_ms
void System_Frugal::periodically() {
  Frugal_Group::periodically();
}

// Main loop() - call this from main.cpp
void System_Frugal::loop() {
  //Serial.print("⏳1");
  if (timeForPeriodic) {
    periodically();  // Do things run once per cycle
    infrequently();  // Do things that keep their own track of time
    timeForPeriodic = false;
  }
  Frugal_Group::loop(); // Do things like MQTT which run frequently with their own clock
  if (powercontroller->maybeSleep()) { // Note this returns true if sleep, OR if period for POWER_MODE_LOOP
    timeForPeriodic = true; // reset after sleep (note deep sleep comes in at top again)
  }
  delay(10); // Slow down and let other things run
}

void System_Frugal::startSerial(uint32_t baud, uint16_t serial_delay) {
  // Encapsulate setting up and starting serial
  #ifdef ANY_DEBUG
    Serial.begin(baud);
    while (!Serial) { 
      ; // wait for serial port to connect. Needed for Arduino Leonardo only
    }
    delay(serial_delay); // If dont do this on D1 Mini and Arduino IDE then miss next debugging
    Serial.println(F("FrugalIoT Starting"));
  #endif // ANY_DEBUG  
}
  
void System_Frugal::startSerial() {
  // At least on my mac, Arduino has problems at higher speeds like 460800
  #ifdef PLATFORMIO
    startSerial(460800, 5000);
  #else
    startSerial(115200, 5000);
  #endif
}

// Called by OTA byt checking other modules
bool System_Frugal::canOTA() {
  return wifi->connected();
}
bool System_Frugal::canMQTT() {
  return mqtt->connected();
}