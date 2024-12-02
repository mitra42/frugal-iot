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

bool output = false;
float input;
float input2 = CONTROL_DEMO_MQTT_HUMIDITY_MAX; // TODO-55 control this through UI slider
String *inputTopic;  // Topic for value checking
String *outputTopic;  // Topic for sending result

void act() {
  if (outputTopic) {
    #ifdef CONTROL_DEMO_MQTT_DEBUG
      Serial.print(F("Control_demo_MQTT: Sending on ")); Serial.print(*outputTopic); Serial.print(F(" ")),Serial.println(output);
    #endif
    xMqtt::messageSend(*outputTopic, output, true, 1); // Note message will probably be queued, as this will be running inside a messageHandler and has qos!=0
  #ifdef CONTROL_DEMO_MQTT_DEBUG
  } else {
    Serial.print(F("Control_demo_MQTT: no outputTopic"));
  #endif
  }
}

void set(boolean o) {
  if (o != output) {
    output = o;
    act();
  }
}

bool decide() {
  return (input > input2); // TODO add hysterisis
}

void inputSet(String *payload) {
  float i = payload->toFloat();
  if (i != input) {
    input = i;
    set(decide());
  }
}
void inputReceived(String *payload) {
  inputSet(payload);
}
// TODO-81 replace all the messageReceived(&topic, &payload) with inputReceived(&payload) etc
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void inputMessageReceived(String &topic, String &payload) {
#pragma GCC diagnostic pop
  inputReceived(&payload);
}

void input2Set(String *payload) {
  float i = payload->toFloat();
  if (i != input2) {
    input2 = i;
    set(decide());
  }
}
void input2Received(String *payload) {
  input2Set(payload);
}

void outputControlReceived(String &payload) {
  if (!outputTopic || *outputTopic != payload) {
    // Only send if its changed.
    outputTopic = new String(payload);
    act();
  }
}

void inputControlSet(String *t) {
  if (!inputTopic || *inputTopic != *t) {
    // Only send if its changed.
    inputTopic = new String(*t);
    // TODO-55 need to unsubscribe from previous topic, or will end up here as well
    xMqtt::subscribe(*inputTopic, *inputMessageReceived); //TODO-81
  }
}
void inputControlReceived(String &payload) {
  inputControlSet(&payload);
}

// TODO-81 replace all the messageReceived(&topic, &payload) with inputReceived(&payload) etc
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void input2MessageReceived(String &topic, String &payload) {
#pragma GCC diagnostic pop
  input2Received(&payload);
}

// TODO-81 replace all the messageReceived(&topic, &payload) with inputReceived(&payload) etc
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void outputControlMessageReceived(String &topic, String &payload) {
#pragma GCC diagnostic pop
  outputControlReceived(payload);
}

// ===== not converted below here  ==================================

void setup() {
  String *t;
  //Input1: Hard coded topic from elsewhere which will send values
  t = new String(*xDiscovery::topicPrefix + F(SENSOR_SHT85_TOPIC_HUMIDITY));  // Hard coded to own humidity - will allow control next
  inputControlSet(t);

  //Input2: Will receive values from the UX
  t = new String(*xDiscovery::topicPrefix + F("control_demo_mqtt_input2")); 
  xMqtt::subscribe(*t, *input2MessageReceived);

  //Output: Go to topic as defined in UX
  t = new String(*xDiscovery::topicPrefix + "control_demo_mqtt_outputControl");
  xMqtt::subscribe(*t, *outputControlMessageReceived);
}

} //namespace cDemoMqtt
#endif // CONTROL_DEMO_MQTT_WANT
