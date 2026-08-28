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
 * 
 * Rules: (TODO)
 * 
 */

#include <Arduino.h>
#include <list>
#include <forward_list>
#include "system/message.h"
#include "system/frugal.h" // for frugal_iot
#include "misc.h" // heap_print
#ifdef ESP32
  #include "esp_task_wdt.h" // TODO-125
#endif


// Note Strings passed must be safe - copy before calling this if going to go out of scope.
System_Message::System_Message(const String& topicPath, const String& payload, const bool retain, const int qos, const uint16_t flags)
: topicPath(topicPath), payload(payload), retain(retain), qos(qos), flags_(flags) {}

// Called implicitly from outgoing.emplace in System_Messages::subscribe
System_Message::System_Message(const String& topicPath) // For subscriptions only
: System_Message(topicPath, String(), false, 0, MsgIsSubscription) {}

System_Messages::System_Messages() 
: System_Base("messages", "Messages"),
topicPrefix()
{ }

void System_Messages::buildTopicPrefix() {
    topicPrefix = frugal_iot.org + F("/") + frugal_iot.project + F("/") + frugal_iot.nodeid + F("/");
    subscribe(path("set/#"));  // Main subscription to all changes sent to this node
}
// Note this setup might be done early (and called twice), rather than in frugal_iot.setup 
void System_Messages::setup() {
  if (!topicPrefix.length()) { // Check if already done
    // Nothing to read from disk so not calling readConfigFromFS 
    // e.g. "dev/developers/esp32-12345/" prefix of most topics
    buildTopicPrefix();
  }
}

void System_Messages::loop() {
  sendOutgoingQueued(); // Upstream
  dispatchIncomingQueued();
}

// =========== Helpers =====================
// Convert a twig e.g. "set/#" to path e.g. dev/developers/esp123/sht30/temperature
String System_Messages::path(char const * const topicTwig) { // TODO find other places do this and replace with call to TopicPath
    setup(); // Allow control wiring before setup by doing setup early
  return topicPrefix + topicTwig;
}
// Convert a twig e.g. "set/#" to path e.g. dev/developers/esp123/sht30/temperature
String System_Messages::setPath(char const * const topicTwig) { // TODO find other places do this and replace with call to TopicPath
    setup(); // Allow control wiring before setup by doing setup early
  return topicPrefix + "set/" + topicTwig;
}
// Convert a twig e.g. sht30/temperature to path e.g. dev/developers/esp123/sht30/temperature
String System_Messages::path(const String topicTwig) { // TODO find other places do this and replace with call to TopicPath
  setup(); // Allow control wiring before setup by doing setup early
  return topicPrefix + topicTwig; // e.g. dev/lotus/esp1234/sht/temperature or .../temperature/max
}
String System_Messages::path(const char* id, char const * const twig) { // TODO find other places do this and replace with call to TopicPath
  setup(); // Allow control wiring before setup by doing setup early
  return topicPrefix + id + "/" + twig; // Note topicPrefix ends in "/"
}
String System_Messages::path(const char* id, const String& twig) { // TODO find other places do this and replace with call to TopicPath
  setup(); // Allow control wiring before setup by doing setup early
  return topicPrefix + id + "/" + twig; // Note topicPrefix ends in "/"
}
String System_Messages::path(const char* id,  const char* const leaf, const char* const leafparm) { // TODO find other places do this and replace with call to TopicPath
  setup(); // Allow control wiring before setup by doing setup early
  return topicPrefix + id + "/" + leaf + "/" + leafparm; // Note topicPrefix ends in "/"
}

// ============ UPSTREAM ====== MODULES -> (queue -> LoRaMesher) -> queue -> MQTT -> Broker 

// Upstream: module => queued
void System_Messages::subscribe(const String topicPath) {
  for(System_Message& sub: subscriptions) {
    if (sub.topicPath == topicPath) {
      return; // Dont resubscribe
    }
  }
  // Send subscription upstream UNLESS its a purely local !set subscription, in which case it can only be created 
  // locally and will be reflected by Messages loopback without a subscription. 
  if ( ! topicPath.startsWith(frugal_iot.messages->topicPrefix) 
       || topicPath.substring(frugal_iot.messages->topicPrefix.length()).startsWith("set/")) {
    outgoing.emplace_back(topicPath); // Implicit new Message (subscription)
  }
}

// Upstream: module => queue outgoing (no reflection)
void System_Messages::sendRemote(const String topicPath, const String payload, bool retain, uint8_t qos, uint16_t flags) {
  // Instead of pushing a new message, update the payload
  for(System_Message& sm: outgoing) {
    if (sm.topicPath == topicPath) {
      #ifdef SYSTEM_MESSAGE_DEBUG
        Serial.print(F("Updating queued ")); Serial.print(topicPath); Serial.print(" "); Serial.print(sm.payload); Serial.print("->"); Serial.println(payload);
      #endif
      #ifdef SYSTEM_MDNS_WANT
        if (sm.payload != payload) {
          sm.flags_ &= ~MsgMdnsDelivered; // Value actually changed - allow mDNS peer delivery to run again
        }
      #endif
      sm.payload = payload;
      return; // Don't push
    }
  }
  #ifdef SYSTEM_MESSAGE_DEBUG
    Serial.print("Queueing "); Serial.print(topicPath); Serial.print("="); Serial.println(payload);
  #endif
  outgoing.emplace_back(topicPath, payload, retain, qos, flags );  // Implicit new Message
}


// Upstream: module => queue for MQTT and queue loopback
void System_Messages::send(const String topicPath, const String payload, bool retain, uint8_t qos) {
  // Note we have to make two messagesas they will be queued (and deleted) separately
  // TODO-210 may need to pass in flags to send, but not clear any but 0x00
  sendRemote(topicPath, payload, retain, qos, 0x00);
  // This does a local loopback, if anything is listening for this message it will get it twice - once locally and once via server.
  queueLoopback(topicPath, payload); 
}

// Upstream queued => MQTT or LoRaMesher (+ mDNS peers, independently, in parallel)
// Every queued message gets a turn each tick, regardless of position, so a message
// MQTT/LoRaMesher can't accept yet doesn't block mDNS delivery - or MQTT/LoRaMesher
// delivery of later messages - behind it.
void System_Messages::sendOutgoingQueued() {
  for (auto it = outgoing.begin(); it != outgoing.end(); ) {
    System_Message& m = *it;
    #ifdef SYSTEM_MDNS_WANT
      if (frugal_iot.mdns) {
        m.attemptMdns(); // Side channel only - doesn't affect whether m is popped below
      }
    #endif
    bool accepted;
    if (m.isSubscription()) {
      accepted = m.queuedSubscribe();
      if (accepted) {
        subscriptions.push_front(m);
      }
    } else {
      accepted = m.queuedMessage();
    }
    if (accepted) {
      it = outgoing.erase(it); // Note this should delete m and free up the memory
    } else {
      ++it;
    }
  }
}
#ifdef SYSTEM_MDNS_WANT
// mDNS peer delivery - retried every loop() until it succeeds (MsgMdnsDelivered),
// independently of MQTT/LoRaMesher below. For a subscription message, notifies the
// matching peer directly (not gated on MQTT/LoRaMesher accepting the subscription).
// For a data message, publishToPeer routes set/ commands to the target device by
// nodeId, and publishToSubscribers pushes to peers that subscribed to this topic.
void System_Message::attemptMdns() {
  if (!(flags_ & MsgMdnsDelivered)) {
    bool delivered;
    if (isSubscription()) {
      delivered = frugal_iot.mdns->notifyPeersOfSubscription(topicPath);
    } else {
      bool toSubscribers = frugal_iot.mdns->publishToSubscribers(topicPath, payload);
      bool toPeer = frugal_iot.mdns->publishToPeer(topicPath, payload, retain, qos);
      delivered = toSubscribers || toPeer;
    }
    if (delivered) {
      flags_ |= MsgMdnsDelivered;
    }
  }
}
#endif

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
    return false; // Nothing else available - stay queued and retry next loop()
  }
}

// Upstream: Outgoing queue => MQTT || LoRaMesher
bool System_Message::queuedSubscribe() {

  if (frugal_iot.mqtt->connected()) {
    return frugal_iot.mqtt->subscribe(topicPath);
  #ifdef SYSTEM_LORAMESHER_WANT
  } else { 
    if (frugal_iot.loramesher && frugal_iot.loramesher->connected()) {
      return frugal_iot.loramesher->publish("subscribe", topicPath,0,1);
    }
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

void System_Message::parse() {
  if (topicPath.startsWith(frugal_iot.messages->topicPrefix)) {
    flags_ |= MsgIsThisNode; // includes trailing slash
  }
  twig_ = topicPath.substring(frugal_iot.messages->topicPrefix.length());  // set/sht/temperature or sht/temperature or set/sht/temperature/max or sht/temperature/max
  if (twig_.startsWith("set/")) {
    flags_ |= MsgIsSet;
    twig_.remove(0, 4); // sht/temperature or sht/temperature/max
  }
  int8_t slashPos = twig_.indexOf('/'); // Find the position of the slash
  module_ = (slashPos == -1) ? String() : twig_.substring(0, slashPos);
  leaf_ = twig_.substring(slashPos + 1);
}
// TODO-210 maybe should be inline
String System_Message::topicTwig() { return twig_; } // sht/temperature or sht/temperature/max
String System_Message::module() { return module_; } // sht
String System_Message::leaf() { return leaf_; } // temperature or temperature/max
bool System_Message::isSet() { return flags_ & MsgIsSet; } 
bool System_Message::isSubscription() { return flags_ & MsgIsSubscription; } 


// Downstream MQTT -> modules (note that LoRaMesher is a module that forwards based on subscriptions)
// Message is on the front of the queue and will be destroyed and memory freed by dispatchIncomingQueued once dispatched
void System_Message::dispatch() {
  parse(); 
  frugal_iot.dispatch(*this); // Send to all modules
}

void System_Message::maybeWriteToFS(bool appendValue) { // appendValue defaults to false
  if (!(flags_ & MsgFromFS)) {
    String path = String("/") + module() + "/" + leaf(); // e.g. sht/temperature or sht/temperature/max
    if (appendValue) {
      path = path + "/value";
    }
    #if defined(SYSTEM_LITTLEFS_DEBUG) || defined(SYSTEM_MESSAGE_DEBUG)
      Serial.print(F("Writing config flags=")); Serial.print(flags_, HEX); Serial.print(F(" ")); Serial.print(path); Serial.print(F("=")); Serial.print(payload);
    #endif
    frugal_iot.fs_LittleFS->spurt(path, payload);
  }
}
void System_Message::maybeEcho() {
  const String path = frugal_iot.messages->path(module() + "/" + leaf());
  #if defined(SYSTEM_MESSAGE_DEBUG)
    Serial.print(F("Echoing flags=")); Serial.print(flags_, HEX); Serial.print(F(" ")); Serial.print(path); Serial.print(F("=")); Serial.println(payload);
  #endif
  frugal_iot.messages->send(path, payload, MQTT_RETAIN, MQTT_QOS_ATLEAST1);
}
void System_Message::maybeWriteToFSandEcho(bool appendValue) { // appendValue defaults to false
  maybeWriteToFS(appendValue);
  maybeEcho();
}

// Downstream queued => dispatch
// Send any messages incoming
void System_Messages::dispatchIncomingQueued() {
  while (!incoming.empty()) {
    System_Message &m = incoming.front();
    m.dispatch();
    incoming.pop_front(); // Note this should delete m and free up the memory
  }
}
// Downstream queued => dispatch
// This is called by either the MQTT or LoRaMesher module on receiving a message
// also by send to implement a loopBack, and by captive
void System_Messages::queueIncoming(const String &topicPath, const String &payload, uint16_t flags) {
  incoming.emplace_back(topicPath, payload, false, 0, flags);  // Implicit new Message (freed in dispatch)
}
void System_Messages::queueFromCaptive(const String &twig, const String &payload) {
  queueIncoming(path(twig), payload, MsgFromCaptive);
}
void System_Messages::queueLoopback(const String &topicPath, const String &payload) {
  queueIncoming(topicPath, payload, MsgIsLoopback);
}

#ifdef SYSTEM_MDNS_WANT
// Called by System_MDNS::onNewPeer() when a new peer is discovered.
// Iterates the local subscription list and sends an HTTP subscribe POST to the
// peer for every topic whose path starts with that peer's org/project/nodeId/ prefix.
void System_Messages::subscribeViaMdns(const String& peerNodeId, IPAddress ip, uint16_t port) {
  String peerPrefix = frugal_iot.org + "/" + frugal_iot.project + "/" + peerNodeId + "/";
  bool anySent = false;
  for (const System_Message& sub : subscriptions) {
    if (sub.topicPath.startsWith(peerPrefix)) {
      #ifdef SYSTEM_MDNS_DEBUG
        Serial.print(F("mDNS: subscribing to ")); Serial.println(sub.topicPath);
      #endif
      frugal_iot.mdns->httpPost(ip, port, "subscribe", sub.topicPath);
      anySent = true;
    }
  }
  #ifdef SYSTEM_MDNS_DEBUG
    if (!anySent) {
      Serial.print(F("mDNS: no local subscriptions match peer ")); Serial.println(peerNodeId);
    }
  #endif
}
#endif // SYSTEM_MDNS_WANT
