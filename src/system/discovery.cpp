/* Manage MQTT higher level 
   Advertise in a way that allows a client to discover the nodes from knowing the project
 
   Periodically (SYSTEM_DISCOVERY_S seconds) send node name on project  e.g  "dev/developers/" = "node1"
  At startup send a YAML string that describes this node and all sensors actuators

  Required SYSTEM_DISCOVERY_S
  Optional SYSTEM_DISCOVERY_DEBUG
  Optional *_WANT and *ADVERTISEMENT for each sensor and actuator

  TODO make this into a class ! 

*/

#include "_settings.h"

#ifndef SYSTEM_DISCOVERY_S
  #define SYSTEM_DISCOVERY_S 30 // quick discovery every 30 seconds
#endif

#include <Arduino.h>
#include "system/discovery.h"
#include "actuator/actuator.h"
#include "sensor/sensor.h"
#include "control/control.h"
#include "system/frugal.h"
#include "misc.h"
#include "system/frugal.h" // for frugal_iot


System_Discovery::System_Discovery()
: System_Base("discovery", "Discovery"),
   timer_index(frugal_iot.powercontroller->timer_next())
{ }

void System_Discovery::quickAdvertise() {
    frugal_iot.messages->send(projectTopic, frugal_iot.nodeid, MQTT_DONT_RETAIN, MQTT_QOS_ATLEAST1); // Don't RETAIN as other nodes also broadcasting to same topic
}

// Exactly the condition System_Message::queuedMessage() uses to accept a message. If neither
// transport is up, nothing fullAdvertise() queues can ever leave the node - it just sits in
// System_Messages::outgoing, which is only ever drained by a successful send.
static bool transportAvailable() {
  bool ok = frugal_iot.canMQTT();
  #ifdef SYSTEM_LORAMESHER_WANT
    if (!ok) {
      ok = (frugal_iot.loramesher && frugal_iot.loramesher->connected());
    }
  #endif
  return ok;
}

  // Tell broker what I've got at start (has to be before quickAdvertise; after sensor & actuator*::setup so can't be inside xDiscoverSetup
  // Relying on short messages from modules instead of large message which won't go thru LoRaMesher
  void System_Discovery::fullAdvertise() {
    //heap_print(F("Discovery::fullAdvertise before"));
    frugal_iot.discover(); // Incremental - does as much as the outgoing queue has room for
    //heap_print(F("Discovery::fullAdvertise after"));
    doneFullAdvertise = frugal_iot.discovered; // Only true once the whole tree has been described
  }

/* Describe the node to the server, a little at a time, once there is somewhere to send it.
 *
 * Two things used to be wrong here, and both showed up as a captive portal that came up blank.
 *
 * FIRST, this ran with no transport check at all, on the reasoning in the old comment in
 * infrequently(): "can queue these up even before MQTT connected as will be sent when connects".
 * That is true of the messages but not of the memory. Nothing drains System_Messages::outgoing
 * until a transport accepts a message, so offline - exactly the state a node is in while someone
 * is using the portal - the queue only grew.
 *
 * SECOND, even connected, fullAdvertise() walked the whole module tree in one call, queueing
 * roughly a hundred System_Message objects before loop() could drain any. On an ESP32-S2 that
 * took free heap from 42,620 bytes to 1,176 and broke the single 98KB free block into fragments
 * under 1.5KB, permanently. WiFi and lwIP need CONTIGUOUS internal memory and cannot use PSRAM,
 * so the portal could build its page and never transmit it, and the AP could not accept a new
 * association. System_Group::discover() is now resumable, and this drives it a few modules at a
 * time.
 *
 * Driven from loop(), not infrequently(): infrequently() can be minutes apart - on a node that deep sleeps it is the whole sleep - so
 * doing part of the tree per infrequently() tick would take hours to describe the node. loop()
 * runs continuously, and System_Messages is ahead of System_Discovery in the system group, so
 * its sendOutgoingQueued() has already drained what it can before this runs again. That is what
 * turns the old single flood into a steady trickle: queue a few, let them go, queue a few more.
 *
 * transportAvailable() is still required. Nothing drains the outgoing queue until a transport
 * accepts a message, so with no transport this would fill the queue to the cap and sit there.
 */
void System_Discovery::loop() {
  if (!doneFullAdvertise && transportAvailable()) {
    fullAdvertise();
  }
}

// Done once after WiFi first connects
void System_Discovery::setup() {
  // Nothing to read from disk so not calling readConfigFromFS 
  projectTopic = frugal_iot.org + "/" + frugal_iot.project;// e.g. "dev/developers"
}

void System_Discovery::infrequently() { 
    #ifdef SYSTEM_DISCOVERY_DEBUG
      Serial.println("System_Discovery: infreq"); // Should trigger each periodic
    #endif
  
  if (frugal_iot.powercontroller->timer_expired(timer_index)) {
    #ifdef SYSTEM_DISCOVERY_DEBUG
      Serial.println("System_Discovery: timer");
    #endif
    // fullAdvertise() used to be driven from here, ungated. It is in loop() now - see there.
    // Even if MQTT down (no WiFi or LoRa) queue up one of these to send when reconnect so UX knows we exist
    quickAdvertise(); // Send info about this node to server (on timer)
    frugal_iot.powercontroller->timer_set(timer_index, SYSTEM_DISCOVERY_S);
  }
}
