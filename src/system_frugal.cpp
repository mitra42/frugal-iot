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


#include "_settings.h" // Note - ideally shouldnt be dependent on anything here
#include "system_frugal.h"
#include "misc.h"
#include "system_group.h"


// Handle messages at top level - check for own, and if not loop through all other modules
// e.g. topicSensorId: "sht30"  topicTwig: "temperature" or "temperature/max"  payload="23.0" 
void System_Frugal::dispatchTwig(const String &topicSensorId, const String &topicLeaf, const String &payload, bool isSet) {
  if (isSet && (topicSensorId == id)) {
    if (topicLeaf == "project") { // TODO unclear we should be changing project on a device live
      project = String(payload); // Note weirdness, it really needs to copy 
      // TODO - needs to redo stuff that uses "project"
      // project = payload;
    } else if (topicLeaf == "name") {
      name = String(payload); // Note weirdness, it really needs to copy 
    } else if (topicLeaf == "description") {
      description = payload;
    }
    writeConfigToFS(topicLeaf, payload); // Save for next time
    System_Base::dispatchTwig(topicSensorId, topicLeaf, payload, isSet);
  } else { // No point in passing on our own id for the loop
    System_Group::dispatchTwig(topicSensorId, topicLeaf, payload, isSet);
  }
}
void System_Frugal::dispatchTwig(const String &topicTwig, const String &payload, bool isSet) {
  // topic Twig  <actuatorId>/<ioID> or  <actuatorId>/<ioID> or <actuatorId>/<ioID>/<config>
  int8_t slashPos = topicTwig.indexOf('/'); // Find the position of the slash
  if (slashPos != -1) {
    String id = topicTwig.substring(0, slashPos);       // Extract the part before the slash
    String topicLeaf = topicTwig.substring(slashPos + 1);      // Extract the part after the slash
    dispatchTwig(id, topicLeaf, payload, isSet); // Start the loop now its been parsed
  } else {
    Serial.print(F("No slash found in topic: ")); Serial.println(topicTwig);
  }
}


void System_Frugal::discover() {
  messages->send(leaf2path("name"), name, MQTT_RETAIN, MQTT_QOS_ATLEAST1);
  messages->send(leaf2path("description"), description, MQTT_RETAIN, MQTT_QOS_ATLEAST1);
  // Commented out because already sending ota_key which contains it.
  //messages->send(leaf2path("board"), SYSTEM_OTA_SUFFIX, MQTT_RETAIN, MQTT_QOS_ATLEAST1);
  System_Group::discover();
}

void System_Frugal::captiveLines(AsyncResponseStream* response) {
  captive->addString(response, id, "project", project, T->Project, 2, 15);
  captive->addString(response, id, "name", name, T->DeviceName, 3, 15);
  captive->addString(response, id, "description", description, T->Description, 3, 40);
  System_Group::captiveLines(response);
}

// Note this constructor is running BEFORE Serial is enabledd
System_Frugal::System_Frugal(const char* org, const char* project, const char* name, const char* description)
: System_Group("frugal_iot", name),
  org(org),
  project(project),
  description(description),
    // Use the unique id of the ESP32 or ESP8266 - has to be before anything calls messages.path
  #ifdef ESP32
    nodeid(String(F("esp32-")) + (Sprintf("%06" PRIx64, ESP.getEfuseMac() >> 24))),
  #elif defined(ESP8266)
    nodeid(String(F("esp8266-")) + (Sprintf("%06" PRIx32, ESP.getChipId()))),
  #else
    #error nodeid Only defined for ESP32 and ESP8266
  #endif
  actuators(new System_Group("actuators", "Actuators")),
  sensors(new System_Group("sensors", "Sensors")),
  controls(new System_Group("controls", "Controls")),
  system(new System_Group("system", "System")),
  buttons(new System_Buttons("buttons", "Buttons")),
  captive(new System_Captive()),
  discovery(new System_Discovery()), 
  #ifdef SYSTEM_LORA_WANT
    lora(new System_LoRa()),
  #endif
    messages(new System_Messages()),
  // mqtt is added in main.cpp > configure_mqtt(host,user,password)
  #ifdef SYSTEM_OLED_WANT // Set in _settings.h on applicable boards or can be added by main.cpp
    oled(new Actuator_OLED(&OLED_WIRE)),
  #endif // SYSTEM_OLED_WANT
  #if defined(SYSTEM_OTA_PREFIX) && defined(SYSTEM_OTA_SUFFIX)
    ota(new System_OTA()),
  #endif
  fs_LittleFS(new System_LittleFS()),
  time(nullptr), // time is optional and setup by main.cpp if needed
  wifi(new System_WiFi())
{
  #ifdef LED_BUILTIN // defined by board or _settings.h
    actuators->add(new Actuator_Ledbuiltin(LED_BUILTIN)); // Default LED builtin actuator at default brightness and white
  #endif
  #ifdef SYSTEM_OLED_WANT // Set in _settings.h on applicable boards or can be added by main.cpp
    actuators->add(oled);
  #endif
  sensors->add(new Sensor_Health("health", "System"));
  add(actuators);
  add(sensors);
  add(controls);
  add(buttons); 
  system->add(fs_LittleFS);
  system->add(messages);
  system->add(wifi);
  system->add(discovery);
  system->add(new System_Watchdog());
  #if defined(SYSTEM_OTA_PREFIX) && defined(SYSTEM_OTA_SUFFIX)
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
void System_Frugal::pre_setup() {
  // Early initial stuff - happens BEFORE do System_Message::setup which uses config 
  startSerial(); // Encapsulate setting up and starting serial
  fs_LittleFS->pre_setup();
  powercontroller->pre_setup(); // Turns on power pin on Lilygo, maybe others
  readConfigFromFS(); // Reads config (project, name) and passes to our dispatchTwig
}
void System_Frugal::setup() {
  // By the time this is run, mqtt should have been added, and serial started in main.cpp
  #ifdef SYSTEM_FRUGAL_DEBUG
    Serial.print(F("Setup: "));
  #endif
  System_Group::setup(); // includes WiFi
  #if defined(SYSTEM_OTA_PREFIX) && defined(SYSTEM_OTA_SUFFIX)
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
  //heap_print(F("infrequent"));
  System_Group::infrequently();
  //heap_print(F("/infrequent"));
}

// These are things done one time per period - where a period is the time set in powercontroller->cycle_ms
void System_Frugal::periodically() {
  System_Group::periodically();
}

// Main loop() - call this from main.cpp
void System_Frugal::loop() {
  //Serial.print(F("⏳1"));
  if (timeForPeriodic) {
    //heap_print(F("loop periodic"));
    periodically();  // Do things run once per cycle
    infrequently();  // Do things that keep their own track of time
    timeForPeriodic = false;
    //heap_print(F("/loop periodic"));
  }
  System_Group::loop(); // Do things like MQTT which run frequently with their own clock
  if (powercontroller->maybeSleep()) { // Note this returns true if sleep, OR if period for POWER_MODE_LOOP
    timeForPeriodic = true; // reset after sleep (note deep sleep comes in at top again)
  }
  delay(10); // Slow down and let other things run
}

void System_Frugal::startSerial(uint32_t baud, uint16_t serial_delay) {
  // Encapsulate setting up and starting serial
  #ifdef ANY_DEBUG
    Serial.begin(baud);
    /*
    while (!Serial) { 
      ; // wait for serial port to connect. Needed for Arduino Leonardo only
    }
    */
    //TODO-23 remove this delay when coming back from deep sleep.
    delay(serial_delay); // If dont do this on D1 Mini and Arduino IDE then miss next debugging
    Serial.println(F("FrugalIoT Starting"));
  #endif // ANY_DEBUG  
}
#ifndef SERIAL_BAUD
  #define SERIAL_BAUD 460800
#endif
  
void System_Frugal::startSerial() {
  // At least on my mac, Arduino has problems at higher speeds like 460800
  #ifdef PLATFORMIO
    startSerial(SERIAL_BAUD, 5000);
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
