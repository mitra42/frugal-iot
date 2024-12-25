/*
  Base class for digital actuators 
*/

#include "_settings.h"  // Settings for what to include etc

// Add new digital actuators to this statement.  #TO_ADD_ACTUATOR
#if defined(ACTUATOR_RELAY_WANT) || defined(ACTUATOR_LEDBUILTIN_WANT) // TODO make this generic, but LED almost always wanted

#include <Arduino.h>
#include "system_mqtt.h"
#include "system_discovery.h" // 
#include "actuator_digital.h" // defines ACUATOR_DIGITAL_DEBUG

Actuator_Digital::Actuator_Digital(const uint8_t p) { pin = p; };

void Actuator_Digital::act() {
  digitalWrite(pin, value ? HIGH : LOW); // Relay pin on Wemos shield is NOT inverted
}
void Actuator_Digital::set(const bool v) {
  value = v;
  #ifdef ACTUATOR_DIGITAL_DEBUG
    Serial.print(F("\nSetting ")); Serial.print(*name); Serial.println(v ? F(" on") : F(" off"));
  #endif
}
void Actuator_Digital::inputReceived(const String &payload) {
  const uint8_t v = payload.toInt(); // Copied to pin in the loop 
  #ifdef ACTUATOR_DIGITAL_DEBUG
    Serial.print(*name); Serial.print(F(" received ")); Serial.println(v);
  #endif
  set(v);
  act();
}

void Actuator_Digital::setup() {
  // initialize the digital pin as an output.
  pinMode(pin, OUTPUT);

  //TODO-C++EXPERT done in actuator_relay.h etc as cant figure out how to do the callback (tried variations of std::bind etc)
  //xMqtt::subscribe(*topic, [this](*topic, String &payload) { this->inputReceived(payload); } );
}


#endif // ACTUATOR_DIGITAL_WANT
