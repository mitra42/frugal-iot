/*
  Demo MQTT by listening for humidity and controlling LED

  Optional CONTROL_DEMO_MQTT_DEBUG 
 */


#include "_settings.h"  // Settings for what to include etc

#ifdef CONTROL_DEMO_MQTT_WANT


#include <Arduino.h>
#include "control_demo_mqtt.h"
#include "actuator_ledbuiltin.h"  // TODO-43 remove dependency once controllable.
#include "system_mqtt.h"
#include "sensor_sht85.h" // TODO-43 remove dependency once controllable.
#include "system_discovery.h"

// TODO-46 genericize this when works - around input. control, output etc  
namespace cDemoMqtt {

bool value = false;
String *inTopic;  // Topic for value checking
String *outTopic;  // Topic for value checking
String *controlOutTopic; // The topic to listen to to hear what topic should be sending to

void act() {
  if (outTopic) {
    #ifdef CONTROL_DEMO_MQTT_DEBUG
      Serial.print(F("Control_demo_MQTT: Sending on ")); Serial.print(*outTopic); Serial.print(F(" ")),Serial.println(value);
    #endif
    xMqtt::messageSend(*outTopic, value, true, 1); // Note message will be queued , and sent outside of messageReceived handler
  #ifdef CONTROL_DEMO_MQTT_DEBUG
  } else {
    Serial.print(F("Control_demo_MQTT: no outTopic"));
  #endif
  }
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void inputMessageReceived(String &topic, String &payload) {
#pragma GCC diagnostic pop
  float humidity = payload.toFloat();
  bool newValue = humidity > CONTROL_DEMO_MQTT_HUMIDITY_MAX;
  // Only send if its changed.
  if (newValue != value) {
    value = newValue;
    act();
  }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void controlOutMessageReceived(String &topic, String &payload) {
#pragma GCC diagnostic pop
  #ifdef CONTROL_DEMO_MQTT_DEBUG
    Serial.print(F("cDemoMqtt received controlOut ")); Serial.println(payload);
  #endif
  if (!outTopic || *outTopic != payload) {
    // Only send if its changed.
    outTopic = new String(payload);
    act();
  }
}
void setup() {
  inTopic = new String(*xDiscovery::topicPrefix + F(SENSOR_SHT85_TOPIC_HUMIDITY));
  xMqtt::subscribe(*inTopic, *inputMessageReceived);
  controlOutTopic = new String(*xDiscovery::topicPrefix + "control_demo_mqtt_output");
  xMqtt::subscribe(*controlOutTopic, *controlOutMessageReceived);
  // act();  // TODO-43 Unsure why this was here - needs to do calcs before value has any meaning 
}

} //namespace cDemoMqtt
#endif // CONTROL_DEMO_MQTT_WANT
