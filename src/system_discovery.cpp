/* Manage MQTT higher level 
   Advertise in a way that allows a client to discover the nodes from knowing the project
 
   Periodically (SYSTEM_DISCOVERY_MS) send node name on project  e.g  "dev/Lotus/" = "node1"
  At startup send a YAML string that describes this node and all sensors actuators

  Required SYSTEM_DISCOVERY_MS 
  Required SYSTEM_DISCOVERY_ORGANIZATION
  Optional SYSTEM_DISCOVERY_DEBUG
  Optional *_WANT and *ADVERTISEMENT for each sensor and actuator

  TODO make this into a class ! 

*/

#include "_settings.h"

#ifdef SYSTEM_DISCOVERY_WANT // Until have BLE, no WIFI means local only

#if !defined(SYSTEM_DISCOVERY_ORGANIZATION)
  #error system_discover does not have all requirements in _locals.h: SYSTEM_DISCOVERY_ORGANIZATION
#endif

#ifndef SYSTEM_DISCOVERY_MS
  #define SYSTEM_DISCOVERY_MS (30000) // quick discovery every 30 seconds
#endif

#include <Arduino.h>
#include "system_discovery.h"
#ifdef ACTUATOR_WANT
  #include "actuator.h"
#endif
#include "sensor.h"
#ifdef CONTROL_WANT
  #include "control.h"
#endif
#include "system_frugal.h"
// For sleepSafemillis()
#include "system_frugal.h" // for frugal_iot


System_Discovery::System_Discovery() 
: System_Base("discovery", "Discovery") { }

void System_Discovery::quickAdvertise() {
    frugal_iot.mqtt->messageSend(*projectTopic,  frugal_iot.wifi->clientid(), false, 0); // Don't RETAIN as other nodes also broadcasting to same topic
}

// TODO-141 see if this divergence is still true.
//const char PROGMEM system_discovery_organization_slash[] = SYSTEM_DISCOVERY_ORGANIZATION "/";
#ifdef ESP8266 // Runtime Exception if try and add char[] to String 
  #define idcolon F("id: ")
  #define nlNameColon F("\nname: ")
  //const char PROGMEM xxx[] = ""; //TODO_C++EXPERT - for weird reason requires this and Serial.print(xxx) or get run time exception
#else // ESP32 - can't start a String concat with a F()
  #define idcolon "id: "
  #define nlNameColon F("\nname: ")
#endif

// Tell broker what I've got at start (has to be before quickAdvertise; after sensor & actuator*::setup so can't be inside xDiscoverSetup
void System_Discovery::fullAdvertise() {
  // Note - this is intentionally not a global string as it can be quite big, better to create, send an free up
  String* advertisePayload = new String( 
    idcolon + frugal_iot.wifi->clientid() 
    + nlNameColon + frugal_iot.wifi->device_name
    + F("\ndescription: "
    // Can be overridden in _local.h
    #ifdef SYSTEM_DISCOVERY_DEVICE_DESCRIPTION
      SYSTEM_DISCOVERY_DEVICE_DESCRIPTION
    #else
      BOARDNAME
    #endif
    #ifdef SYSTEM_OTA_WANT // TODO-141 get this from presence or absence of ota_key. 
      "\nota: " SYSTEM_OTA_KEY 
    #endif
    // TODO-44 add location: <gsm coords>
    "\ntopics:" 
    )
  );
  *advertisePayload += frugal_iot.advertisement();
  frugal_iot.mqtt->messageSend(*advertiseTopic, *advertisePayload, true, 1);
  doneFullAdvertise = true;
}

void System_Discovery::setup_after_mqtt() { // TODO-141 move this to MQTT after that is in frugal_iot
  // This line fails when board 'LOLIN C3 PICO' is chosen
  // projectTopic = new String(F(SYSTEM_DISCOVERY_ORGANIZATION "/") + frugal_iot.wifi->discovery_project + F("/"));
  //Serial.print(xxx); //TODO_C++EXPERT - for weird reason requires this and const char PROGMEM above  or get run time exception
  projectTopic = new String(SYSTEM_DISCOVERY_ORGANIZATION "/" + frugal_iot.wifi->discovery_project );
  advertiseTopic = new String(*projectTopic + F("/") + frugal_iot.wifi->clientid()); // e.g. "dev/lotus/esp32-12345"
  topicPrefix = new String(*advertiseTopic + F("/")); // e.g. "dev/lotus/esp32-12345/" prefix of most topics
  #ifdef SYSTEM_DISCOVERY_DEBUG
    Serial.print(F("topicPrefix=")); Serial.println(*topicPrefix);
  #endif
  // Subscribe to all `set` for this node
  frugal_iot.mqtt->subscribe("set/#"); 
}

 //TODO-23 This wont work as nextLoopTime wont be remembered in Deep Sleep
void System_Discovery::infrequently() { 
    if (nextLoopTime <= (frugal_iot.powercontroller->sleepSafeMillis())) {
      if (!doneFullAdvertise) {
        fullAdvertise();
      } 
      quickAdvertise(); // Send info about this node to server (on timer)
      nextLoopTime = frugal_iot.powercontroller->sleepSafeMillis() + SYSTEM_DISCOVERY_MS;
    }
}

#endif // SYSTEM_DISCOVERY_WANT

