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
#include "misc.h"
// For sleepSafemillis()
#include "system_frugal.h" // for frugal_iot


System_Discovery::System_Discovery()
: System_Base("discovery", "Discovery") { }

void System_Discovery::quickAdvertise() {
    frugal_iot.messages->send(projectTopic, frugal_iot.nodeid, MQTT_DONT_RETAIN, MQTT_QOS_ATLEAST1); // Don't RETAIN as other nodes also broadcasting to same topic
}

  // Tell broker what I've got at start (has to be before quickAdvertise; after sensor & actuator*::setup so can't be inside xDiscoverSetup
  // Relying on short messages from modules instead of large message which won't go thru LoRaMesher
  void System_Discovery::fullAdvertise() {
    heap_print(F("Discovery::fullAdvertise before"));
    frugal_iot.discover();
    heap_print(F("Discovery::fullAdvertise after"));
    doneFullAdvertise = true;
  }

// Done once after WiFi first connects
void System_Discovery::setup() {
  // Nothing to read from disk so not calling readConfigFromFS 
  projectTopic = frugal_iot.org + "/" + frugal_iot.project;
  advertiseTopic = projectTopic + F("/") + frugal_iot.nodeid; // e.g. "dev/developers/esp32-12345"
}

 //TODO-23 This wont work as nextLoopTime wont be remembered in Deep Sleep
void System_Discovery::infrequently() { 
    if  (nextLoopTime <= (frugal_iot.powercontroller->sleepSafeMillis())) {
      if ((!doneFullAdvertise)) { // (&& frugal_iot.canMQTT()) - should be able to do this over LoRaMesher
        // Can queue these up even before MQTT connected as will be sent when connects
        fullAdvertise();
      } 
      // quick can be fone if have MQTT or have LoRaMesher
      //Serial.print("XXX" __FILE__); Serial.println(__LINE__);
      if (frugal_iot.canMQTT() 
        #ifdef SYSTEM_LORAMESHER_WANT // This is automatically defined on LoRa compatable boardss
          || (frugal_iot.loramesher && frugal_iot.loramesher->connected())
        #endif
      ) {
        quickAdvertise(); // Send info about this node to server (on timer)
      }
      nextLoopTime = frugal_iot.powercontroller->sleepSafeMillis() + SYSTEM_DISCOVERY_MS;
    }
}
