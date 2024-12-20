/*
  Turn the relay on or off

  Required from .h: ACTUATOR_RELAY_TOPIC
  Optional:  ACTUATOR_RELAY_PIN ACTUATOR_RELAY_DEBUG
*/

#include "_settings.h"  // Settings for what to include etc

#ifdef ACTUATOR_RELAY_WANT

#include <Arduino.h>
#include "actuator_relay.h"
#include "actuator_digital.h"
#include "system_mqtt.h"
#include "system_discovery.h"

#ifndef ACTUATOR_RELAY_PIN
  #ifdef ESP8266_D1_MINI
    // Reasonable to assume using the Wemos Relay shield
    #define ACTUATOR_RELAY_PIN D1 // Default on Wemos relay shield for D1 mini or C3 Pico 
  #else 
    #ifdef LOLIN_C3_PICO
      // Reasonable to assume using the Wemos Relay shield
      #define ACTUATOR_RELAY_PIN 10 // Not sure if pin 10 is called "10" in digital write
    #else
      #error Need to define ACTUATOR_RELAY_PIN for unknown boards
    #endif
  #endif
#endif // ACTUATOR_RELAY_PIN

namespace aRelay {

Actuator_Digital actuator_relay(ACTUATOR_RELAY_PIN);

// TODO-C++EXPERT I cant figure out how to pass the class Actuator_Digital.inputReceived as callback, have tried various combinstiaons of std::bind but to no success
void inputReceived(String &payload) {
  actuator_relay.inputReceived(payload);
}
void setup() {
  #ifdef ACTUATOR_DIGITAL_DEBUG
      actuator_relay.name = new String(F("relay"));
  #endif // ACTUATOR_DIGITAL_DEBUG
  actuator_relay.topic = String(*xDiscovery::topicPrefix + ACTUATOR_RELAY_TOPIC);
  actuator_relay.setup();
  xMqtt::subscribe(actuator_relay.topic, *inputReceived); // TODO-C++EXPERT see comment above
}

// void loop() { }

} //namespace aRelay
#endif // ACTUATOR_RELAY_WANT
