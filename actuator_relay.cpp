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
#include "actuator_digital.h"
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

Actuator_Digital actuator_relay(ACTUATOR_RELAY_PIN);

// TODO-C++EXPERT I cant figure out how to pass the class Actuator_Digital.messageReceived as callback, have tried various combinstiaons of std::bind but to no success
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void messageReceived(String &topic, String &payload) {
#pragma GCC diagnostic ignored "-Wunused-parameter"
  actuator_relay.messageReceived(topic, payload);
}
void setup() {
  #ifdef ACTUATOR_DIGITAL_DEBUG
    *actuator_relay.name = F("relay");
  #endif // ACTUATOR_DIGITAL_DEBUG
  actuator_relay.topic = String(*xDiscovery::topicPrefix + ACTUATOR_RELAY_TOPIC);
  actuator_relay.setup();
  xMqtt::subscribe(actuator_relay.topic, *messageReceived); // TODO-C++EXPERT see comment above
}

// void loop() { }

} //namespace aRelay
#endif // ACTUATOR_RELAY_WANT
