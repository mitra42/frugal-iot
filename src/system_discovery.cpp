/* Manage MQTT higher level 
   Advertise in a way that allows a client to discover the nodes from knowing the project
 
   Periodically (SYSTEM_DISCOVERY_MS) send node name on project  e.g  "dev/developers/" = "node1"
  At startup send a YAML string that describes this node and all sensors actuators

  Required SYSTEM_DISCOVERY_MS 
  Optional SYSTEM_DISCOVERY_DEBUG
  Optional *_WANT and *ADVERTISEMENT for each sensor and actuator

  TODO make this into a class ! 

*/

#include "_settings.h"

#ifndef SYSTEM_DISCOVERY_MS
  #define SYSTEM_DISCOVERY_MS (30000) // quick discovery every 30 seconds
#endif

#include <Arduino.h>
#include "system_discovery.h"
#include "actuator.h"
#include "sensor.h"
#include "control.h"
#include "system_frugal.h"
// For sleepSafemillis()
#include "system_frugal.h" // for frugal_iot


System_Discovery::System_Discovery()
: System_Base("discovery", "Discovery") { }

void System_Discovery::quickAdvertise() {
    frugal_iot.mqtt->messageSend(*projectTopic,  frugal_iot.nodeid, false, 0); // Don't RETAIN as other nodes also broadcasting to same topic
}

// Tell broker what I've got at start (has to be before quickAdvertise; after sensor & actuator*::setup so can't be inside xDiscoverSetup
void System_Discovery::fullAdvertise() {
  // Note - this is intentionally not a global string as it can be quite big, better to create, send an free up
  String* advertisePayload = new String( 
    "id: " + frugal_iot.nodeid // ESP32 doesnt like F("id:") for first arg (ESP8266 is fine)
    + F("\nname: ") + frugal_iot.device_name
    + F("\ndescription: ") + frugal_iot.description
    #ifdef SYSTEM_OTA_KEY
    + F("\nota: " SYSTEM_OTA_KEY)
    #endif
    // TODO-44 add location: <gsm coords>
    + F("\ntopics:") 
  );
  *advertisePayload += frugal_iot.advertisement();
  frugal_iot.mqtt->messageSend(*advertiseTopic, *advertisePayload, true, 1);
  doneFullAdvertise = true;
}

// Done once after WiFi first connects
void System_Discovery::setup() {
  projectTopic = new String(frugal_iot.org + "/" + frugal_iot.project );
  advertiseTopic = new String(*projectTopic + F("/") + frugal_iot.nodeid); // e.g. "dev/developers/esp32-12345"
}

 //TODO-23 This wont work as nextLoopTime wont be remembered in Deep Sleep
 // TODO-153 as done now will only check once (infrequently() runs once per period), and if it runs before MQTT it will fail and not retry
void System_Discovery::infrequently() { 
    if (frugal_iot.canMQTT() && (nextLoopTime <= (frugal_iot.powercontroller->sleepSafeMillis()))) {
      if (!doneFullAdvertise) {
        fullAdvertise();
      } 
      quickAdvertise(); // Send info about this node to server (on timer)
      nextLoopTime = frugal_iot.powercontroller->sleepSafeMillis() + SYSTEM_DISCOVERY_MS;
    }
}
