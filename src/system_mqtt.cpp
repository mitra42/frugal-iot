/* MQTT client
* 
* Configuration
* Required: SYSTEM_MQTT_MS
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
* messageReceived -> dispatch -> (dispatchTwig; dispatchPath)
*/

#include "_settings.h"
#ifdef ESP32
  #include "esp_task_wdt.h" // TODO-125
#endif

#include <MQTT.h>
#include "system_frugal.h" // for frugal_iot

#ifndef SYSTEM_MQTT_BACKOFF
  #define SYSTEM_MQTT_BACKOFF 5000 // Reasonable backoff on MQTT conncetion failure - 5 seconds (was 10ms ! )
#endif
// mqtt client -> System_MQTT callback
// Note intentionally outside class, passed as callback to Mqtt client
void MqttMessageReceived(String &topicPath, String &payload) { // cant be constant as dispatch isnt
  frugal_iot.mqtt->messageReceived(topicPath, payload);
}

// Run every 10ms TODO-25 and TODO-23 this should be MUCH longer ideally
System_MQTT::System_MQTT(const char* hostname, const char* username, const char* password) 
: System_Base("mqtt", "MQTT"), 
  client(1024,128),
  hostname(hostname),
  ms(10),
  nextLoopTime(0), 
  password(password),
  username(username)
{}

void System_MQTT::setup() {
  readConfigFromFS(); // Reads config (hostname) and passes to our dispatchTwig
}

// Setup MQTT, connect and subscribe - note if WiFi is connected, this will block till MQTT times out 
void System_MQTT::setup_after_wifi() {
  // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported
  // by Arduino. You need to set the IP address directly.
  client.begin(hostname.c_str(), net);
  client.setCleanSession(true); // on power up should refresh subscriptions
  // client.setClockSource(XXX); // TODO-23 See https://github.com/256dpi/arduino-mqtt will need for power management
  client.onMessage(MqttMessageReceived);  // Called back from client.loop - this is a naked function that just calls into the instance
  connect(); // Note if WiFi is connected, this will block till MQTT times out 
}
void System_MQTT::captiveLines(AsyncResponseStream* response) {
  frugal_iot.captive->addString(response, id, "hostname", hostname, T->MQTThostname, 5, 60);
}

void System_MQTT::loop() {
  if (nextLoopTime <= millis()) {
    // Automatically reconnect
    if (connect()) { ; // If Wifi is connected, this is blocking till timeout
      if (!client.loop()) {
        #ifdef SYSTEM_MQTT_DEBUG
          // https://github.com/256dpi/lwmqtt/blob/master/include/lwmqtt.h#L15
          // TODO-125 unclear which error this reports  it might client.returnCode()
          Serial.print(F("MQTT client loop failed ")); Serial.println(client.lastError()); // lwmqtt_err
        #endif // SYSTEM_MQTT_DEBUG
      }; // Do this at end of loop so some time before checks if connected
      nextLoopTime = millis() + SYSTEM_MQTT_MS; // Not sleepSafeMillis as this is frequent
    } else {
      nextLoopTime = (millis() + SYSTEM_MQTT_BACKOFF);
    }
  }
}
// ========== HELPERS ======================
bool System_MQTT::connected() {
  return client.connected(); 
}
/* Connect to MQTT broker and - if necessary - resubscribe to all topics */
// Note this is blocking if not connected, and Wifi is connected // TODO-153 flag in callers.
bool System_MQTT::connect() {
  if (!frugal_iot.wifi->connected()) { //TODO-150 maybe its ok if have LoRaMesher instead
    //Serial.print(F("MQTT waiting for WiFi"));
    return false; // State machine should be reconnecting
  }
  if (!client.connected()) {
    // TODO-153 make this non blocking
    /* Not connected */
    Serial.print(F("\nMQTT connecting: to ")); Serial.print(hostname);
    // The call to client.connect is blocking 
    // Theoretically "skip=true" should be good, dont close if connected, but leads to error code=6
    if (!client.connect(frugal_iot.nodeid.c_str(), username, password)) {
      /* Still not connected */
      Serial.print(F(" Fail "));
      Serial.print(client.lastError()); // -3 is LWMQTT_NETWORK_FAILED_CONNECT
      Serial.print(F(" ")); // 6 is LWMQTT_UNKNOWN_RETURN_CODE 
      // https://github.com/256dpi/lwmqtt/blob/master/include/lwmqtt.h#L116
      Serial.println(client.returnCode());
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
    if (frugal_iot.messages->reSubscribeAll()) {
      subscriptionsDone = true;
    } else {
      return false; // Something failed - probably connection dropped.
    }
  }
  /* Connected and Subscriptions done */
  client.setCleanSession(false);  // Next time use the session created
  return true;
}
// This is for MQTT messages addressed at the mqtt module e.g. dev/org/node/set/mqtt/hostname
void System_MQTT::dispatchTwig(const String &topicSensorId, const String &topicTwig, const String &payload, bool isSet) {
  if (isSet && (topicSensorId == id)) {
    if (topicTwig == "hostname") {
      hostname = payload;
      writeConfigToFS(topicTwig, payload);
    } else {
      System_Base::dispatchTwig(topicSensorId, topicTwig, payload, isSet);
    }
  }
}
bool System_MQTT::prepareForLightSleep() {
  frugal_iot.mqtt->client.disconnect();
  return true;
}
bool System_MQTT::recoverFromLightSleep() {
  return connect(); // TODO-23 Note this is blocking if WiFi is connected, which it typically won't be. 
}

// UPSTREAM module -> queue -> (loRaMesher -> queue ) -> MQTT -> Broker

bool System_MQTT::subscribe(const String& topicPath) {
  if (connected()) {
      #ifdef SYSTEM_MQTT_DEBUG
        Serial.print(F("MQTT Subscribe ")); Serial.print(topicPath);
      #endif
    if (client.subscribe(topicPath)) {
      #ifdef SYSTEM_MQTT_DEBUG
        Serial.println();
      #endif
      return true; 
    } else {
      // Note at this point we don't have a fallback if happen to subscribe when MQTT fails
      #ifdef SYSTEM_MQTT_DEBUG
        Serial.print(F(" Failed"));
        // https://github.com/256dpi/lwmqtt/blob/master/include/lwmqtt.h#L15
        Serial.println(client.lastError());
      #endif // SYSTEM_MQTT_DEBUG
      // Drop thru to return false
    }
  }
  return false;
}
// If retain is set, then the broker will keep a copy 
// TODO implement qos on broker in this library
// qos: 0 = send at most once; 1 = send at least once; 2 = send exactly once
// These are intentionally required parameters rather than defaulting so the coder thinks about the desired behavior
bool System_MQTT::send(const String &topicPath, const String &payload, const bool retain, const int qos) {
  if (connected()) {
    #ifdef SYSTEM_MQTT_DEBUG
      Serial.print(topicPath); Serial.print(F("=")); Serial.print(payload); 
      // Serial.print(" qos="); Serial.print(qos);
    #endif
    if (client.publish(topicPath, payload, retain, qos)) {
      #ifdef SYSTEM_MQTT_DEBUG
        Serial.println();
      #endif
      return true;
    } else {
      #ifdef SYSTEM_MQTT_DEBUG
        Serial.print(F("Failed to publish: ")); 
      #endif
      // https://github.com/256dpi/lwmqtt/blob/master/include/lwmqtt.h#L15
        
      switch (client.lastError()) {
        case -1:
          #ifdef SYSTEM_MQTT_DEBUG
            Serial.print(F(" MQTT Buffer too small, message length~")); Serial.println(topicPath.length() + payload.length());
          #endif
          return true; // It failed, but won't succeed later so delete
          //break;
        case -9:
          #ifdef SYSTEM_MQTT_DEBUG
            Serial.println(F(" Missing or Wrong packet"));
          #endif
          break;
        default: 
          #ifdef SYSTEM_MQTT_DEBUG
            Serial.print(F(" err=")); Serial.println(client.lastError());
          #endif
        }
      return false;
    };
  } else { 
    return false; // Not connected to MQTT so leave on queue
  }
}

// ========== DOWNSTREAM: Broker -> Mqtt -> (LoRaMesher) -> Modules


void System_MQTT::messageReceived(const String &topicPath, const String &payload) { // cant be constant as dispatch isnt
  #ifdef SYSTEM_MQTT_DEBUG
    Serial.print(F("MQTT incoming: ")); Serial.print(topicPath); Serial.print(F("=")); Serial.println(payload);
  #endif
  inReceived = true;
  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
  frugal_iot.messages->dispatch(topicPath, payload);
  inReceived = false;
}




