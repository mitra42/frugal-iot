/* MQTT client
* 
* Configuration
* Required: SYSTEM_MQTT_MS SYSTM_MQTT_USER SYSTEM_MQTT_PASSWORD
* Optional: ESP8266 SYSTEM_MQTT_DEBUG 
* 
* Note definitions
* topicPath = full path /dev/project/node/sensor/leaf and usually String or String& or String*
* topicPrefix = /dev/project/node/ (note trailng slash)
* topicTwig (or Twig) is the components after the topicPrefix typically sensor/io and usually char*
* topicLeaf (or Leaf) is the last component e.g. "temperature" and usually char* (deprecated in favor of topicTwig) its "id" in any IO
* "topic" is ambiguous and therefore wrong ! 
*
* Incoming flow 
* messageReceived -> dispatch -> (dispatchTwigAll; dispatchPathAll)
*/

#include "_settings.h"
#ifdef ESP32
  #include "esp_task_wdt.h" // TODO-125
#endif

#if (!defined(SYSTEM_MQTT_USER) || !defined(SYSTEM_MQTT_PASSWORD))
  error system_discover does not have all requirements in _locals.h: SYSTEM_MQTT_USER SYSTEM_MQTT_PASSWORD 
#endif

#define SYSTEM_MQTT_LOOPBACK // If true dispatch the message locally as well - this is always the case currerntly 

#if ESP8266 // Note ESP8266 and ESP32 are defined for respective chips - unclear if anything like that for other Arduinos
  #include <ESP8266WiFi.h>  // for WiFiClient
#else
  #include <WiFi.h> // This will be platform dependent, will work on ESP32 but most likely want configurration for other chips/boards
#endif

#include <MQTT.h>
#include "misc.h" // For StringF

// If configred not to use Wifi (or in future BLE) then will just operate locally, sending MQTT between components on this node, but 
// not elsewhere.
// TODO-49 add support for BLE if it makes sense for MQTT
#ifdef SYSTEM_WIFI_WANT
  #include "system_wifi.h"   // xWifi
#endif  //SYSTEM_WIFI_WANT
#include <Arduino.h>
#include "system_discovery.h"
#include "system_mqtt.h"
#include "actuator.h"
#include "sensor.h"
#include "control.h"
//TODO-25 replace with control.h when ready
  #ifdef CONTROL_BLINKEN_WANT
    #include "control_blinken.h"
  #endif
#include <forward_list>

Subscription::Subscription(const String* const tp) : topicPath(tp), payload(NULL) { }
Subscription::Subscription(const String* const tp, String const* pl) : topicPath(tp), payload(pl) { }

bool Subscription::operator==(const String& tp) {
  return *topicPath == tp;
}
Message::Message(const String &tp, String const &pl, const bool r, const int q): Subscription(&tp, &pl), retain(r), qos(q) { }

MqttManager* Mqtt; // Will get initialized by setup in frugalIot.ino


void MqttManager::setup() {
  // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported
  // by Arduino. You need to set the IP address directly.
  client.begin(xWifi::mqtt_host.c_str(), net);
  client.setCleanSession(true); // on power up should refresh subscriptions
  // client.setClockSource(XXX); // TODO-23 See https://github.com/256dpi/arduino-mqtt will need for power management
  client.onMessage(xMqtt::MessageReceived);  // Called back from client.loop - this is a naked function that just calls into the instance
  blockTillConnected();
}
// Run every 10ms TODO-25 and TODO-23 this should be MUCH longer ideally
MqttManager::MqttManager() : Frugal_Base(), client(1024,128), nextLoopTime(0), ms(10) {
  setup();
}

void MqttManager::frequently() {
  if (nextLoopTime <= millis()) {
    // Automatically reconnect
    blockTillConnected(); // TODO-125 maybe make non blocking and queue messages while down
    messageSendQueued();
    if (!client.loop()) {
      #ifdef SYSTEM_MQTT_DEBUG
        // https://github.com/256dpi/lwmqtt/blob/master/include/lwmqtt.h#L15
        // TODO-125 unclear which error this reports  it might client.returnCode()
        Serial.print(F("MQTT client loop failed ")); Serial.println(client.lastError()); // lwmqtt_err
      #endif // SYSTEM_MQTT_DEBUG
    }; // Do this at end of loop so some time before checks if connected
    nextLoopTime = millis() + SYSTEM_MQTT_MS; // Not sleepSafeMillis as this is frequent
  }
}

/* Connect to MQTT broker and - if necessary - resubscribe to all topics */
bool MqttManager::connect() {
  xWifi::checkConnected();  // TODO-22 - blocking and potential puts portal up, may prefer some kind of reconnect
  if (!client.connected()) {
    /* Not connected */
    Serial.print(F("\nMQTT connecting: to ")); Serial.print(xWifi::mqtt_host.c_str());
    if (!client.connect(xWifi::clientid().c_str(), SYSTEM_MQTT_USER, SYSTEM_MQTT_PASSWORD)) {
      /* Still not connected */
      Serial.print(F(" Fail "));
      // https://github.com/256dpi/lwmqtt/blob/master/include/lwmqtt.h#L116
      Serial.print(client.returnCode());
      return false;
    } else { 
      /* Fresh connection */
      Serial.print(F(" Connected "));
      if (!client.sessionPresent()) {
        subscriptionsDone = false; // No session so will need to redo subscriptions
      } else {
        Serial.println(F(" Session present "));
      }
    }
  }
  /* Have a connection - new or old */
  if (!subscriptionsDone) { // Client has reported existence of a session
    /* State connected but broker doesnt know subscriptions */
    if (resubscribeAll()) { 
      subscriptionsDone = true;
    } else {
      return false; // Something failed - probably connection dropped.
    }
  }
  /* Connected and Subscriptions done */
  client.setCleanSession(false);  // Next time use the session created
  return true;
}

/* Connect to MQTT, loop until succeed */
void MqttManager::blockTillConnected() {
  while (!connect()) {
    #ifdef ESP32
      esp_task_wdt_reset();
    #endif
    delay(1000); // Block waiting for WiFi and MQTT to connect 
  }
}

Subscription* MqttManager::find(const String &topicPath) {
  for(Subscription& mi: subscriptions) {
    if (mi == topicPath) {
      return &mi;
    }
  }
  return NULL;
}

void MqttManager::subscribe(const String& topicPath) {
  #ifdef SYSTEM_MQTT_DEBUG
    Serial.print(F("Subscribing to: ")); Serial.println(topicPath);
  #endif
  Subscription* mi = find(topicPath);
  if (mi) { // No existing subscription
    if (mi->payload) { // If have retained previous data
      messageReceived(*mi->topicPath, *mi->payload); // TODO-25 check for loops or wasted internal duplicates
    }
  } else { 
    if (!client.subscribe(topicPath)) {
      #ifdef SYSTEM_MQTT_DEBUG
        Serial.println(F("MQTT Subscription failed to ")); Serial.print(topicPath); Serial.print(F(" "));
        // https://github.com/256dpi/lwmqtt/blob/master/include/lwmqtt.h#L15
        Serial.println(client.lastError());
      #endif // SYSTEM_MQTT_DEBUG
    }
    subscriptions.emplace_front(&topicPath); // Should create a Subscription
  }
}

String* MqttManager::path(char const * const topicTwig) { // TODO find other places do this and replace with call to TopicPath
  return new String(*xDiscovery::topicPrefix + topicTwig);
}
String* MqttManager::twig(const String &topicPath) { 
  if (topicPath.startsWith(*xDiscovery::topicPrefix)) {
    String* const topicTwig = new String(topicPath);
    topicTwig->remove(0, xDiscovery::topicPrefix->length());
    return topicTwig;
  } else {
    return nullptr;
  }
}
// Short cut to allow subscribing based on an actuator or sensors own topic
void MqttManager::subscribe(const char* topicTwig) {
  const String * const topicPath = path(topicTwig);
  subscribe(*topicPath);
}
void MqttManager::dispatch(const String &topicPath, const String &payload) {
  // TODO move this to _base.cpp
  if (topicPath.startsWith(*xDiscovery::topicPrefix)) { // includes trailing slash
    String topicTwig = topicPath.substring(xDiscovery::topicPrefix->length()); 
    bool isSet;
    if (topicTwig.startsWith("set/")) {
      isSet = true;
      topicTwig.remove(0, 4); // Remove set/ from start
      // At the moment it looks like all dispatchTwig are isSet, because control subscribes to full path for its wired.
      #ifdef SENSOR_WANT
        Sensor::dispatchTwigAll(topicTwig, payload, isSet); // None of the sensors have subscriptions
      #endif
      #ifdef ACTUATOR_WANT
        Actuator::dispatchTwigAll(topicTwig, payload, isSet); // Just matches twigs
      #endif
      #ifdef CONTROL_WANT
        // note called for isSet and !isSet
        Control::dispatchTwigAll(topicTwig, payload, isSet); // Just matches twigs
      #endif
    }
  }
 
  #ifdef CONTROL_WANT
    Control::dispatchPathAll(topicPath, payload);  // Matches just paths. Twigs and sets handle above
  #endif
  //TODO-25 System::dispatchPathAll(*topicPath, payload)
}
bool MqttManager::resubscribeAll() {
  // TODO-125 may put a flag on subscriptions then only resubscribe those not done
  // TODO-125 should probably check connected each time go around loop and only flag if sendInner succeeds
  Serial.print(F("Resubscribing: ")); 
  for (Subscription mi : subscriptions) {
    Serial.print(*mi.topicPath); Serial.print(F(" "));
    if (!client.subscribe(*(mi.topicPath))) {
      // https://github.com/256dpi/lwmqtt/blob/master/include/lwmqtt.h#L15
      Serial.print(F("FAILED ")); Serial.print(client.lastError()); Serial.println(F(" "));
      return false; // If fails there is either a coding problem. Or connection not working - don't keep pushing
    }
    #ifdef ESP32
      esp_task_wdt_reset();
    #endif
  }
  Serial.println(); delay(1000);
  return true;
}

void MqttManager::retainPayload(const String &topicPath, const String &payload) {
  Subscription* mi = find(topicPath);
  if (mi) {
    mi->payload = new String(payload); // TODO maybe a mem leak - the prev value not explicitly destroyed but no references left to it
  }
}

void MqttManager::messageReceived(const String &topicPath, const String &payload) { // cant be constant as dispatch isnt
  #ifdef SYSTEM_MQTT_DEBUG
    Serial.print(F("MQTT incoming: ")); Serial.print(topicPath); Serial.print(F(" - ")); Serial.println(payload);
  #endif
  inReceived = true;
  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
  dispatch(topicPath, payload);
  inReceived = false;
}

// If retain is set, then the broker will keep a copy 
// TODO implement qos on broker in this library
// qos: 0 = send at most once; 1 = send at least once; 2 = send exactly once
// These are intentionally required parameters rather than defaulting so the coder thinks about the desired behavior

// Send message to Mqtt client - used for both repeats and first time messages
void MqttManager::messageSendInner(const String &topicPath, const String &payload, const bool retain, const int qos) {
  if (!client.publish(topicPath, payload, retain, qos)) {
    #ifdef SYSTEM_MQTT_DEBUG
      Serial.print(F("Failed to publish: ")); Serial.print(topicPath); Serial.print(F("=")); Serial.print(payload); 
      Serial.print(" qos="); Serial.print(qos);
      // https://github.com/256dpi/lwmqtt/blob/master/include/lwmqtt.h#L15
      
      switch (client.lastError()) {
        case -1:
          Serial.print(F(" MQTT Buffer too small, message length~")); Serial.println(topicPath.length() + payload.length());
          break;
        case -9:
          Serial.println(F(" Missing or Wrong packet"));
          break;
        default: 
          Serial.print(F(" err=")); Serial.println(client.lastError());
      }
    #endif // SYSTEM_MQTT_DEBUG
    if (qos > 0) {
      // This doesn't work - if first publish failed, this does, and it loops
      //Serial.print(F("Requeuing"));
      //queued.emplace_front(topicPath, payload, retain, qos);
    }
  };
}

// Send or queue up a message 
void MqttManager::messageSend(const String &topicPath, const String &payload, const bool retain, const int qos) {
  // TODO-21-sema also queue if WiFi is down and qos>0 - not worth doing till xWifi::connect is non-blocking
  #ifdef SYSTEM_MQTT_DEBUG
    Serial.print(F("MQTT ")); Serial.print((inReceived && qos) ? F("queue ") : F("publish ")); Serial.print(topicPath); Serial.print(F(" - ")); Serial.println(payload);
  #endif
  if (inReceived && qos) {
    const String* topicPathCopy = new String(topicPath); // payload will go out of scope before queue flushed
    const String* payloadCopy = new String(payload); // payload will go out of scope before queue flushed
    queued.emplace_front(*topicPathCopy, *payloadCopy, retain, qos); 
  } else {
    messageSendInner(topicPath, payload, retain, qos);
  }
  // Whether send to net or queue, send loopback and do the retention stuff. 
  if (retain) {
    retainPayload(topicPath, payload); // Keep a copy of outgoing, so local subscribers will see 
  }
  // This does a local loopback, if anything is listening for this message it will get it twice - once locally and once via server.
  dispatch(topicPath, payload);
}


// Be careful if change this to avoid either out-of-scope or memory leaksx xxxxxx
void MqttManager::messageSend(const char* const topicTwig, const String &payload, const bool retain, const int qos) {
  const String topicPath = String(*xDiscovery::topicPrefix + topicTwig); // TODO can merge into next line
  messageSend(topicPath, payload, retain, qos);
}

void MqttManager::messageSend(const String &topicPath, const float &value, const int width, const bool retain, const int qos) {
  const String foo = String(value, width);
  messageSend(topicPath, foo, retain, qos);
}
void MqttManager::messageSend(const char* const topicTwig, const float &value, const int width, const bool retain, const int qos) {
  const String foo = String(value, width);
  messageSend(topicTwig, foo, retain, qos);
}
void MqttManager::messageSend(const String &topicPath, const int value, const bool retain, const int qos) {
  const String foo = String(value); Serial.println(foo);
  messageSend(topicPath, foo, retain, qos);
}
void MqttManager::messageSend(const char* const topicTwig, const int value, const bool retain, const int qos) {
  const String foo = String(value);
  messageSend(topicTwig, foo, retain, qos);
}
void MqttManager::messageSend(const String &topicPath, const bool value, const bool retain, const int qos) {
  const String foo = String(value); Serial.println(foo);
  messageSend(topicPath, foo, retain, qos);
}
void MqttManager::messageSend(const char* const topicTwig, const bool value, const bool retain, const int qos) {
  const String foo = String(value);
  messageSend(topicTwig, foo, retain, qos);
}
void MqttManager::messageSendQueued() {
  // TODO-125 should probably check connected each time go around loop and only pop if sendInner succeeds
  while (!queued.empty()) {
    Serial.print("XXX queue not empty " __FILE__); Serial.println(__LINE__);
    Message &m = queued.front();
    messageSendInner(*m.topicPath, *m.payload, m.retain, m.qos);
    queued.pop_front();
    //TODO-125 prob need to delete message popped
  }
}
bool MqttManager::prepareForLightSleep() {
  return true;
}
bool MqttManager::recoverFromLightSleep() {
  return connect();
}
namespace xMqtt {

// Note intentionally outside class, passed as callback to Mqtt client
void MessageReceived(String &topicPath, String &payload) { // cant be constant as dispatch isnt
  Mqtt->messageReceived(topicPath, payload);
}
} // namespace xMqtt
