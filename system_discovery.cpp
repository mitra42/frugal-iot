/* Manage MQTT higher level 
   Advertise in a way that allows a client to discover the nodes from knowing the project
 
   Periodically (SYSTEM_DISCOVERY_MS) send node name on project  e.g  "dev/Lotus/" = "node1"
   When a client sends "?" to "dev/Lotus/"
   Respond with a YAML string that describes this node and all sensors actuators

  Required SYSTEM_DISCOVERY_MS 
  Required SYSTEM_DISCOVERY_ORGANIZATION // TODO-29 will move to configuration
  Optional SYSTEM_DISCOVERY_DEBUG
  Optional *_WANT and *ADVERTISEMENT for each sensor and actuator

  //TODO-29 rename this to system_discovery.cpp 
*/

#include "_settings.h"

#ifdef SYSTEM_DISCOVERY_WANT // Until have BLE, no WIFI means local only
#if (!defined(SYSTEM_DISCOVERY_MS) || !defined(SYSTEM_DISCOVERY_ORGANIZATION))
  error system_discover does not have all requirements in _configuration.h: SYSTEM_DISCOVERY_MS SYSTEM_DISCOVERY_ORGANIZATION
#endif

#include <Arduino.h>
#include "_common.h"    // Main include file for Framework
#include "system_wifi.h"
#include "system_mqtt.h"  // xMqtt
#include "system_discovery.h"

namespace xDiscovery {

unsigned long nextLoopTime = 0;

//TODO Optimization - should these be String & instead of String *
String *projectTopic;
String *advertiseTopic;
String *topicPrefix;

String *advertisePayload = new String(
    SYSTEM_DISCOVERY_ADVERTISEMENT
    #ifdef ACTUATOR_LEDBUILTIN_WANT
      ACTUATOR_LEDBUILTIN_ADVERTISEMENT
    #endif
    #ifdef SENSOR_SHT85_WANT
      SENSOR_SHT85_ADVERTISEMENT
    #endif
  );
void quickAdvertise() {
    xMqtt::messageSend(*projectTopic,  xWifi::clientid(), false, 0); // Don't RETAIN as other nodes also broadcasting to same topic
}

//TODO-29 want retained upstream but not local - non trivial
void fullAdvertise() {
  xMqtt::messageSend(*advertiseTopic, *advertisePayload, true, 1); //TODO-29 should fbe retain=true qos=1
}
/*
void messageReceived(String &topic, String &payload) {
  #ifdef SYSTEM_DISCOVERY_DEBUG
    Serial.print("Discovery message receieved:");
  #endif // SYSTEM_DISCOVERY_DEBUG
  // topic can only be advertiseTopic so no need to test
  if (payload[0] == '?') {
    #ifdef SYSTEM_DISCOVERY_DEBUG
      Serial.println("Request for advertisement");
    #endif
    fullAdvertise();
  }
}
*/

void setup() {
  projectTopic = new String(SYSTEM_DISCOVERY_ORGANIZATION "/" + xWifi::discovery_project + "/"); // e.g. "dev/Lotus Ponds/" TODO-29 will come from configure
  advertiseTopic = new String(*projectTopic + xWifi::clientid()); // e.g. "dev/Lotus Ponds/esp32-12345"     TODO-29 will come from configure
  topicPrefix = new String(*advertiseTopic + "/"); // e.g. "dev/Lotus Ponds/esp32-12345/" prefix of most topics

  #ifdef SYSTEM_DISCOVERY_DEBUG
    Serial.print("topicPrefix="); Serial.println(*topicPrefix);
  #endif
    fullAdvertise(); // Tell broker what I've got at start (note, intentionally before quickAdvertise) 
    // xMqtt::subscribe(*advertiseTopic, *messageReceived); // Commented out as don't see why need to receive this
}

void loop() {
    if (nextLoopTime <= millis()) {
        quickAdvertise(); // Send info about this node to server (on timer)
        nextLoopTime = millis() + SYSTEM_DISCOVERY_MS;
    }
}

} // namespace xDiscovery
#endif // SYSTEM_DISCOVERY_WANT

