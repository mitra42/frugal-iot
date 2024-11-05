/*
  Turn the relay on or off

  Required from .h: ACTUATOR_RELAY_TOPIC
  Optional:  ACTUATOR_RELAY_PIN ACTUATOR_RELAY_DEBUG

  TODO merge with actuator_ledbuiltin, maybe use a class
*/

#include "_settings.h"  // Settings for what to include etc

#ifdef ACTUATOR_RELAY_WANT

#include <Arduino.h>
#include "actuator_relay.h"
#include "system_mqtt.h"
#include "system_discovery.h"

#ifndef ACTUATOR_RELAY_PIN
  #if defined(LOLIN_C3_PICO) || defined(ESP8266_D1_MINI)
    // Reasonable to assume using the Wemos Relay shield
    #define ACTUATOR_RELAY_PIN D1 // Default on Wemos relay shield for D1 mini or C3 Pico 
  #else
    #error Need to define ACTUATOR_RELAY_PIN for unknown boards
  #endif
#endif // ACTUATOR_RELAY_PIN

namespace aRelay {

bool value;  // 1 for on, 0 for off.

void set(int v) {
  value = v;
  #ifdef ACTUATOR_RELAY_DEBUG
    Serial.print(F("\nSetting Relay ")); Serial.println(v ? F("on") : F("off"));
  #endif // ACTUATOR_RELAY_DEBUG

  digitalWrite(ACTUATOR_RELAY_PIN, v ? HIGH : LOW); // Relay pin on Wemos shield is NOT inverted
}

//TODO-53 maybe should be &topic
String *topic;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void messageReceived(String &topic, String &payload) {
#pragma GCC diagnostic pop
  uint8_t v = payload.toInt(); // Copied to pin in the loop 
  #ifdef ACTUATOR_RELAY_DEBUG
    Serial.print(F("aRelay received "));
    Serial.println(v);
  #endif
  set(v);
}

void setup() {           
  topic = new String(*xDiscovery::topicPrefix + ACTUATOR_RELAY_TOPIC);
  // initialize the digital pin as an output.
  pinMode(ACTUATOR_RELAY_PIN, OUTPUT);
  xMqtt::subscribe(*topic, *messageReceived);
}

// void loop() { }

} //namespace aLEDBUILTIN
#endif // ACTUATOR_RELAY_WANT
