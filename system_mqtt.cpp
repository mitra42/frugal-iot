/* MQTT client
* 
* Configuration
* Required: SYSTEM_MQTT_MS SYSTEM_MQTT_PASSWORD //TODO-25 check
* Optional: ESP8266 SYSTEM_MQTT_DEBUG SYSTEM_WIFI_WANT SYSTEM_MQTT_LOOPBACK //TODO-25 check
* 
* Note definitions
* topicpath = full path /dev/project/node/topicleaf and usually String or String& or String*
* topicleaf is the last component e.g. "temperature" and usually char*
* "topic" is ambiguous and therefore wrong ! 
*/

#include "_settings.h"

#if (!defined(SYSTEM_MQTT_USER) || !defined(SYSTEM_MQTT_PASSWORD) || !defined(SYSTEM_MQTT_MS))
  error system_discover does not have all requirements in _configuration.h: SYSTEM_DISCOVERY_MS 
#endif

#if ESP8266 // Note ESP8266 and ESP32 are defined for respective chips - unclear if anything like that for other Arduinos
  #include <ESP8266WiFi.h>  // for WiFiClient
#else
  #include <WiFi.h> // This will be platform dependent, will work on ESP32 but most likely want configurration for other chips/boards
#endif

#include <MQTT.h>

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
//TODO-25 replace with control.h when ready
  #ifdef CONTROL_BLINKEN_WANT
    #include "control_blinken.h"
  #endif
  #ifdef CONTROL_DEMO_MQTT_WANT
    #include "control_demo_mqtt.h"
  #endif
#include <forward_list>

Subscription::Subscription(const String* const tp) : topicpath(tp), payload(NULL) { }
Subscription::Subscription(const String* const tp, String const* pl) : topicpath(tp), payload(pl) { }

bool Subscription::operator==(const String& tp) {
  Serial.print("Subscription:"); Serial.print(*topicpath); Serial.print(*topicpath == tp ? "==" : "!="); Serial.println(tp); // TODO-25 remove when debugged
  return *topicpath == tp;
}
Message::Message(const String &tp, String const &pl, const bool r, const int q): Subscription(&tp, &pl), retain(r), qos(q) { }

MqttManager* Mqtt; // Will get initialized by setup in frugalIot.ino


void MqttManager::setup() {
  // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported
  // by Arduino. You need to set the IP address directly.
  client.begin(xWifi::mqtt_host.c_str(), net);
  client.onMessage(xMqtt::MessageReceived);  // Called back from client.loop - this is a naked function that just calls into the instance

  // Note WiFi should be connected by this point but will check here anyway
  while (!connect()) {
    #ifdef SYSTEM_MQTT_DEBUG
      Serial.print(F("."));
    #endif
    delay(1000); // Block waiting for WiFi and MQTT to connect 
  }
}

// Run every 10ms TODO-25 and TODO-23 this should be MUCH longer ideally
MqttManager::MqttManager() : Frugal_Base(), nextLoopTime(0), ms(10) {
  setup();
}

void MqttManager::loop() {
  if (nextLoopTime <= millis()) {
    // Automatically reconnect
    if (!client.connected()) {
      if (!connect()) { // Non blocking but skip client.loop. Note if fails to connect will set nextLoopTime in 1000 ms.
        nextLoopTime = millis() + 1000; // If non-blocking then dont do any MQTT for a second then try connect again
      }
    } else {
      messageSendQueued();
      if (!client.loop()) {
        #ifdef SYSTEM_MQTT_DEBUG
          Serial.print(F("MQTT client loop failed ")); Serial.println(client.lastError()); // lwmqtt_err
        #endif // SYSTEM_MQTT_DEBUG
      }; // Do this at end of loop so some time before checks if connected
      nextLoopTime = millis() + SYSTEM_MQTT_MS;
    }
  }
}

bool MqttManager::connect() {
  xWifi::checkConnected();  // TODO-22 - blocking and potential puts portal up, may prefer some kind of reconnect
  if (client.connected()) {
    return true;
  } else {
    #ifdef SYSTEM_MQTT_DEBUG
      Serial.print(F("\nMQTT connecting: to ")); Serial.print(xWifi::mqtt_host.c_str());
    #endif 
   
    // Each organization needs a password in mosquitto_passwords which can be added by Mitra using mosquitto_passwd
    if (client.connect(xWifi::clientid().c_str(), SYSTEM_MQTT_USER, SYSTEM_MQTT_PASSWORD)) {
      #ifdef SYSTEM_MQTT_DEBUG
        Serial.println(F("Connected"));
      #endif
      resubscribeAll();
      return true;
    } else {
      return false;
    }
  }
}
Subscription* MqttManager::find(const String &topicpath) {
  for(Subscription& mi: items) {
    if (mi == topicpath) {
      return &mi;
    }
  }
  return NULL;
  /*
  // TODO_C++_EXPERT I think following should work, but I've not used std::find or iterators on further_list before so not sure why this (copied from example I found) wont work
  // error: conversion from 'std::_Fwd_list_iterator<Subscription>' to non-scalar type 'Subscription' requested
    Subscription mi = std::find(items.begin(), items.end(), topicpath);
    return mi == items.end() ? NULL : mi;
  */
}

void MqttManager::subscribe(const String& topicpath) {
  #ifdef SYSTEM_MQTT_DEBUG
    Serial.print(F("Subscribing to: ")); Serial.println(topicpath);
  #endif
  Subscription* mi = find(topicpath);
  if (mi) { // No existing subscription
    if (mi->payload) { // If have retained previous data
      messageReceived(*mi->topicpath, *mi->payload); // TODO-25 check for loops or wasted internal duplicates
    }
  } else { 
    if (!client.subscribe(topicpath)) {
      #ifdef SYSTEM_MQTT_DEBUG
        Serial.println(F("MQTT Subscription failed to ")); Serial.print(topicpath);
      #endif // SYSTEM_MQTT_DEBUG
    }
    items.emplace_front(&topicpath); // Should create a Subscription
  }
}

// Short cut to allow subscribing based on an actuator or sensors own topic
void MqttManager::subscribe(const char* topicleaf) {
  const String * const topicpath = new String(*xDiscovery::topicPrefix + topicleaf);
  subscribe(*topicpath);
}
void MqttManager::dispatch(const String &topicpath, const String &payload) {
  if (topicpath.startsWith(*xDiscovery::topicPrefix)) {
    String* const topicleaf = new String(topicpath);
    topicleaf->remove(0, xDiscovery::topicPrefix->length());
    //Sensor::dispatchAll(*topicleaf, payload);
    #ifdef ACTUATOR_WANT
      Actuator::dispatchAll(*topicleaf, payload);
    #endif
  //TODO-25 temporary hack till Control::dispatchAll readu
      #ifdef CONTROL_DEMO_MQTT_WANT
      cDemoMqtt::dispatchLeaf(*topicleaf, payload);
      #endif
    }
  //TODO-25 Control::dispatchAll(*topicpath, payload);
  //TODO-25 temporary hack till Control::dispatchAll readu
    #ifdef CONTROL_BLINKEN_WANT
      cBlinken::dispatch(topicpath, payload);
    #endif
  //TODO-25 temporary hack till Control::dispatchAll readu
    #ifdef CONTROL_DEMO_MQTT_WANT
      cDemoMqtt::dispatchPath(topicpath, payload);
    #endif
  //TODO-25 System::dispatchAll(*topicpath, payload)
}
void MqttManager::resubscribeAll() {
  #ifdef SYSTEM_MQTT_DEBUG
    Serial.print(F("Resubscribing: ")); 
  #endif // SYSTEM_MQTT_DEBUG
  for (Subscription mi : items) {
    #ifdef SYSTEM_MQTT_DEBUG
      Serial.print(*mi.topicpath); Serial.print(F(" "));
    #endif // SYSTEM_MQTT_DEBUG
    if (!client.subscribe(*(mi.topicpath))) {
      #ifdef SYSTEM_MQTT_DEBUG
        Serial.print(F("FAILED "));
      #endif // SYSTEM_MQTT_DEBUG
    }
  }
  #ifdef SYSTEM_MQTT_DEBUG
    Serial.println();
  #endif // SYSTEM_MQTT_DEBUG
}

void MqttManager::retainPayload(const String &topicpath, const String &payload) {
  Subscription* mi = find(topicpath);
  if (mi) {
    mi->payload = new String(payload);
  }
}

void MqttManager::messageReceived(const String &topic, const String &payload) { // cant be constant as dispatch isnt
  #ifdef SYSTEM_MQTT_DEBUG
    Serial.print(F("MQTT incoming: ")); Serial.print(topic); Serial.print(F(" - ")); Serial.println(payload);
  #endif
  inReceived = true;
  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
  dispatch(topic, payload);
  inReceived = false;
}

// If retain is set, then the broker will keep a copy 
// TODO implement qos on broker in this library
// qos: 0 = send at most once; 1 = send at least once; 2 = send exactly once
// These are intentionally required parameters rather than defaulting so the coder thinks about the desired behavior

// Send message to Mqtt client - used for both repeats and first time messages
void MqttManager::messageSendInner(const String &topicpath, const String &payload, const bool retain, const int qos) {
  if (!client.publish(topicpath, payload, retain, qos)) {
    #ifdef SYSTEM_MQTT_DEBUG
      Serial.print(F("Failed to publish ")); Serial.print(topicpath); Serial.print(F("=")); Serial.println(payload);
    #endif // SYSTEM_MQTT_DEBUG
  };
}

// Send or queue up a message 
void MqttManager::messageSend(const String &topicpath, const String &payload, const bool retain, const int qos) {
  // TODO-21-sema also queue if WiFi is down and qos>0 - not worth doing till xWifi::connect is non-blocking
  #ifdef SYSTEM_MQTT_DEBUG
    Serial.print(F("MQTT ")); Serial.print((inReceived && qos) ? F("queue ") : F("publish ")); Serial.print(topicpath); Serial.print(F(" - ")); Serial.println(payload);
  #endif
  if (inReceived && qos) {
    queued.emplace_front(topicpath, payload, retain, qos);
  } else {
    messageSendInner(topicpath, payload, retain, qos);
  }
  // Whether send to net or queue, send loopback and do the retention stuff. 
  if (retain) {
    retainPayload(topicpath, payload); // Keep a copy of outgoing, so local subscribers will see 
  }
  // This does a local loopback, if anything is listening for this message it will get it twice - once locally and once via server.
  dispatch(topicpath, payload);
}

void MqttManager::messageSend(const char* const topicleaf, const String &payload, const bool retain, const int qos) {
  const String * const topicpath = new String(*xDiscovery::topicPrefix + topicleaf); // TODO can merge into next line
  messageSend(*topicpath, payload, retain, qos);
}

void MqttManager::messageSend(const String &topicpath, const float &value, const int width, const bool retain, const int qos) {
  const String * const foo = new String(value, width);
  messageSend(topicpath, *foo, retain, qos);
}
void MqttManager::messageSend(const char* const topicleaf, const float &value, const int width, const bool retain, const int qos) {
  const String * const foo = new String(value, width);
  messageSend(topicleaf, *foo, retain, qos);
}
void MqttManager::messageSend(const String &topicpath, const int value, const bool retain, const int qos) {
  const String * const foo = new String(value);
  messageSend(topicpath, *foo, retain, qos);
}
void MqttManager::messageSend(const char* const topicleaf, const int value, const bool retain, const int qos) {
  const String * const foo = new String(value);
  messageSend(topicleaf, *foo, retain, qos);
}
void MqttManager::messageSendQueued() {
  while (!queued.empty()) {
    Message &m = queued.front();
    messageSendInner(*m.topicpath, *m.payload, m.retain, m.qos);
    queued.pop_front();
  }
}
namespace xMqtt {

// Note intentionally outside class, passed as callback to Mqtt client
void MessageReceived(String &topic, String &payload) { // cant be constant as dispatch isnt
  Mqtt->messageReceived(topic, payload);
}
} // namespace xMqtt
