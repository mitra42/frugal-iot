/*
  Demo MQTT by listening for humidity and controlling LED

  Optional CONTROL_DEMO_MQTT_DEBUG 
 */


#include "_settings.h"  // Settings for what to include etc

#ifdef CONTROL_DEMO_MQTT_WANT


#include <Arduino.h>
#include "control_demo_mqtt.h"
#include "system_mqtt.h"
#include "system_discovery.h"

// TODO-46 genericize this when works - around input. control, output etc  
namespace cDemoMqtt {

//TODO-25 temporary hack till control_dispatchAll ready
const char* const control_demo_mqtt_input2 = "control_demo_mqtt_input2";
const char* const control_demo_mqtt_outputControl = "control_demo_mqtt_outputControl";

bool output = false;
float input;
float input2 = CONTROL_DEMO_MQTT_HUMIDITY_MAX;
String *inputTopic;  // Topic for value checking - will be strings as can be external to this node
String *outputTopic;  // Topic for sending result - will be strings as can be external to this node

void act() {
  if (outputTopic) {
    #ifdef CONTROL_DEMO_MQTT_DEBUG
      Serial.print(F("Control_demo_MQTT: Sending on ")); Serial.print(*outputTopic); Serial.print(F(" ")),Serial.println(output);
    #endif
    Mqtt->messageSend(*outputTopic, output, true, 1); // Note message will probably be queued, as this will be running inside a messageHandler and has qos!=0
  #ifdef CONTROL_DEMO_MQTT_DEBUG
  } else {
    Serial.print(F("Control_demo_MQTT: no outputTopic"));
  #endif
  }
}

void set(const boolean o) {
  if (o != output) {
    output = o;
    act();
  }
}

bool decide() {
  return (input > input2); // TODO add hysterisis
}

void inputSet(String payload) {
  float i = payload.toFloat();
  if (i != input) {
    input = i;
    set(decide());
  }
}
void inputReceived(const String &payload) {
  inputSet(payload);
}

void input2Set(String payload) {
  float i = payload.toFloat();
  if (i != input2) {
    input2 = i;
    set(decide());
  }
}
void input2Received(const String &payload) {
  input2Set(payload);
}

void outputControlReceived(const String &payload) {
  if (!outputTopic || *outputTopic != payload) {
    // Only send if its changed.
    outputTopic = new String(payload);
    act();
  }
}

void inputControlSet(const String *t) {
  if (!inputTopic || *inputTopic != *t) {
    // Only subscribe if its changed.
    inputTopic = new String(*t);
    // TODO-55 need to unsubscribe from previous topic, or will end up here as well
    Mqtt->subscribe(*inputTopic); //TODO-81
  }
}
void inputControlReceived(const String &payload) {
  inputControlSet(&payload);
}

// TODO-25 temporary patch till new control.cpp ready
void dispatchLeaf(const String &topicleaf, const String &payload) {
  if (topicleaf == control_demo_mqtt_input2) {
    input2Received(payload);
  }
  if (topicleaf == control_demo_mqtt_outputControl) {
    outputControlReceived(payload);
  }
}
// TODO-25 temporary patch till new control.cpp ready
void dispatchPath(const String &topicpath, const String &payload) {
  if (inputTopic && (topicpath == *inputTopic)) {
    inputReceived(payload);
  }  
}

// ===== not converted below here  ==================================

void setup() {
  String *t;
  //Input1: Hard coded topic from elsewhere which will send values
  t = new String(*xDiscovery::topicPrefix + F("humidity"));  // Hard coded to own humidity - will allow control next
  inputControlSet(t);

  //Input2: Will receive values from the UX
  Mqtt->subscribe(control_demo_mqtt_input2);

  //Output: Go to topic as defined in UX
  Mqtt->subscribe( control_demo_mqtt_outputControl);
}

} //namespace cDemoMqtt
#endif // CONTROL_DEMO_MQTT_WANT
