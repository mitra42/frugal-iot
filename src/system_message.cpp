/* Frugal IoT - message class
 *
 * This is a common class used by MQTT and LoraMesher 
 * 
 * Messages are either:
 * Upstream. Module -> outgoing queue -> MQTT or LoRaMesher
 * Relayed Upstream: LoRaMesher -> MQTT -> Broker
 * 
 * And for Upstream each of these is split between subscriptions and 
 * 
 * or Downstream Broker -> MQTT -> module
 * or Relayed Downstream: MQTT -> LoRaMesher -> Module 
 *
 * There is also some reflection - i.e. where an Upstream message is reflected downstream 
 * to a different subscriber instead of going via the broker. 
 */

#include <Arduino.h>
#include <list>
#include <forward_list>
#include "system_message.h"
#include "system_frugal.h" // for frugal_iot
#include "misc.h" // heap_print
#ifdef ESP32
  #include "esp_task_wdt.h" // TODO-125
#endif


// Note Strings passed must be safe - copy before calling this if going to go out of scope.
System_Message::System_Message(const String& topicPath, const String& payload, const bool retain, const int qos, const bool isSubscription)
: topicPath(topicPath), payload(payload), isSubscription(isSubscription), retain(retain), qos(qos) {}

System_Message::System_Message(const String& topicPath) // For subscriptions only
: System_Message(topicPath, String(), false, 0, true) {}

System_Messages::System_Messages() 
: System_Base("messages", "Messages"),
topicPrefix(nullptr)
{ }

// Note this setup might be done early (and called twice), rather than in frugal_iot.setup 
void System_Messages::setup() {
  if (!topicPrefix) { // Check if already done
    // e.g. "dev/developers/esp32-12345/" prefix of most topics
    topicPrefix = new String(frugal_iot.org + F("/") + frugal_iot.project + F("/") + frugal_iot.nodeid + F("/"));
    subscribe(path("set/#"));  // Main subscription to all changes sent to this node
  }
}

void System_Messages::loop() {
  sendQueued(); // Upstream
}

// =========== Helpers =====================
// Convert a twig e.g. "set/#" to path e.g. dev/developers/esp123/sht30/temperature
String* System_Messages::path(char const * const topicTwig) { // TODO find other places do this and replace with call to TopicPath
    setup(); // Allow control wiring before setup by doing setup early
  return new String(*topicPrefix + topicTwig);
}
// Convert a twig e.g. sht30/temperature to path e.g. dev/developers/esp123/sht30/temperature
String* System_Messages::path(const String* topicTwig) { // TODO find other places do this and replace with call to TopicPath
  setup(); // Allow control wiring before setup by doing setup early
  return new String(*topicPrefix + *topicTwig);
}
String* System_Messages::path(const char* id, char const * const twig) { // TODO find other places do this and replace with call to TopicPath
  setup(); // Allow control wiring before setup by doing setup early
  return new String(*topicPrefix + id + "/" + twig); // Note topicPrefix ends in "/"
}
String* System_Messages::path(const char* id,  const char* const leaf, const char* const leafparm) { // TODO find other places do this and replace with call to TopicPath
  setup(); // Allow control wiring before setup by doing setup early
  return new String(*topicPrefix + id + "/" + leaf + "/" + leafparm); // Note topicPrefix ends in "/"
}

// Convert a path e.g. /dev/developers/esp123/sht30/temperature to a twig e.g. sht30/temperature 
String* System_Messages::twig(const String &topicPath) { 
  if (topicPath.startsWith(*topicPrefix)) {
    String* const topicTwig = new String(topicPath); // TODO would this work with a substr ? 
    topicTwig->remove(0, topicPrefix->length());
    return topicTwig;
  } else {
    return nullptr;
  }
}

// ============ UPSTREAM ====== MODULES -> (queue -> LoRaMesher) -> queue -> MQTT -> Broker 

// Upstream: module => queued
void System_Messages::subscribe(const String* topicPath) {
  for(System_Message& sub: subscriptions) {
    if (sub.topicPath == *topicPath) {
      return; // Dont resubscribe
    }
  }
  outgoing.emplace_back(*topicPath); // // Implicit new Message (subscription)
}

// Upstream: module => queue with reflection 
void System_Messages::send(const String* topicPath, const String* payload, bool retain, uint8_t qos) {
  outgoing.emplace_back(*topicPath, *payload, retain, qos);  // Implicit new Message
  //TODO-152 dedupe before adding
  // This does a local loopback, if anything is listening for this message it will get it twice - once locally and once via server.
  frugal_iot.messages->dispatch(*topicPath, *payload);
}


// Upstream queued => MQTT or LoRaMesher
// Send any messages waiting to go (subscriptions or messages)
void System_Messages::sendQueued() {
  while (!outgoing.empty()) {
    System_Message &m = outgoing.front();
    if (m.isSubscription) {
      if (m.queuedSubscribe()) {
        subscriptions.push_front(m);
        outgoing.pop_front(); // Note this should delete m and free up the memory
      } else {
        return; // Dont block if not connected
      }
    } else {
      if (m.queuedMessage()) {
        outgoing.pop_front(); // Note this should delete m and free up the memory
      } else {
        return; // Dont block if not connected
      }
    }
    // If succeeded then try and send any other queued messages
  }
}
// Upstream: queued => MQTT or LoRaMesher
bool System_Message::queuedMessage() {
  if (frugal_iot.mqtt->connected()) {
    // This will be false if fail to send, true if either send or its unsendable (too big)
    return frugal_iot.mqtt->send(topicPath, payload, retain, qos);
  #ifdef SYSTEM_LORAMESHER_WANT
  } else if (frugal_iot.loramesher && frugal_iot.loramesher->connected()) {
    return frugal_iot.loramesher->publish(topicPath, payload, retain, qos);
  #endif
  } else {
    return false; // Unable to send, leave on queue 
  }
}

// Upstream: Outgoing queue => MQTT || LoRaMesher
bool System_Message::queuedSubscribe() {
  if (frugal_iot.mqtt->connected()) {
    return frugal_iot.mqtt->subscribe(topicPath);
  #ifdef SYSTEM_LORAMESHER_WANT
  } else if (frugal_iot.loramesher && frugal_iot.loramesher->connected()) {
    return frugal_iot.loramesher->publish("subscribe", topicPath,0,1);
  #endif
  } 
  return false; // Not connected, or failed to send over connection
}

// MQTT calls this when it has re-established a connection and no session is found. 
bool System_Messages::reSubscribeAll() {
  // TODO-125 may put a flag on subscriptions then only resubscribe those not done
  // TODO-125 should probably check connected each time go around loop and only flag if sendInner succeeds
  Serial.print(F("Resubscribing: ")); 
  for (System_Message sub : subscriptions) {
    Serial.print(sub.topicPath); Serial.print(F(" "));
    if (!frugal_iot.mqtt->subscribe(sub.topicPath)) {
      // https://github.com/256dpi/lwmqtt/blob/master/include/lwmqtt.h#L15
      Serial.println(F("FAILED "));
      return false; // If fails there is either a coding problem. Or connection not working - don't keep pushing
    }
    #ifdef ESP32
      esp_task_wdt_reset();
    #endif
  }
  Serial.println(); // delay(1000);
  return true;
}

// ============ DOWNSTREAM ====== Broker -> MQTT -> (LoRaMesher) -> Modules

// Downstream MQTT -> modules (note that LoRaMesher is a module that forwards based on subscriptions)
// This is called by either the MQTT or LoRaMesher module on receiving a message
void System_Messages::dispatch(const String &topicPath, const String &payload) {
  if (topicPath.startsWith(*topicPrefix)) { // includes trailing slash
    String topicTwig = topicPath.substring(topicPrefix->length()); 
    bool isSet;
    if (topicTwig.startsWith("set/")) {
      isSet = true;
      topicTwig.remove(0, 4); // Remove set/ from start
      // At the moment it looks like all dispatchTwig are isSet, because control subscribes to full path for its wired.
      frugal_iot.dispatchTwig(topicTwig, payload, isSet); // Just matches twigs
    }
  }
  // Note LoRaMesher will go through this to System_LoRaMesher::dispatchPath
  frugal_iot.dispatchPath(topicPath, payload);  // Matches just paths. (Twigs and sets handled above)
}



