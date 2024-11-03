/* Manage MQTT higher level 
   Advertise in a way that allows a client to discover the nodes from knowing the project
 
   Periodically (SYSTEM_DISCOVERY_MS) send node name on project  e.g  "dev/Lotus/" = "node1"
   When a client sends "?" to "dev/Lotus/"
   Respond with a YAML string that describes this node and all sensors actuators

  Required SYSTEM_DISCOVERY_MS 
  Required SYSTEM_DISCOVERY_ORGANIZATION
  Optional SYSTEM_DISCOVERY_DEBUG
  Optional *_WANT and *ADVERTISEMENT for each sensor and actuator

*/

#include "_settings.h"

#ifdef SYSTEM_DISCOVERY_WANT // Until have BLE, no WIFI means local only
#if (!defined(SYSTEM_DISCOVERY_MS) || !defined(SYSTEM_DISCOVERY_ORGANIZATION))
  #error system_discover does not have all requirements in _configuration.h: SYSTEM_DISCOVERY_MS SYSTEM_DISCOVERY_ORGANIZATION
#endif

#ifndef SYSTEM_DISCOVERY_DEVICE_DESCRIPTION
  // add new boards english description here.
  #ifdef ESP8266_D1_MINI
    #define TEMP1 "ESP8266 D1 Mini"
  #else
    #ifdef LOLIN_C3_PICO
      #define TEMP1 "Lolin C3 Pico"
    #else
      #error undefined board in system_discovery.cpp #TO_ADD_NEW_BOARD
    #endif
  #endif
  #ifdef SENSOR_SHT85_WANT
    #define TEMP2 " with SHTxx temp/humidity"
  #else
    #define TEMP2 ""
  #endif
  #define SYSTEM_DISCOVERY_DEVICE_DESCRIPTION TEMP1 TEMP2
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

String *advertisePayload;
void quickAdvertise() {
    xMqtt::messageSend(*projectTopic,  xWifi::clientid(), false, 0); // Don't RETAIN as other nodes also broadcasting to same topic
}

//TODO-29 want retained upstream but not local - non trivial
void fullAdvertise() {
  xMqtt::messageSend(*advertiseTopic, *advertisePayload, true, 1);
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
  projectTopic = new String(SYSTEM_DISCOVERY_ORGANIZATION "/" + xWifi::discovery_project + "/");
  advertiseTopic = new String(*projectTopic + xWifi::clientid()); // e.g. "dev/Lotus Ponds/esp32-12345"
  topicPrefix = new String(*advertiseTopic + "/"); // e.g. "dev/Lotus Ponds/esp32-12345/" prefix of most topics
  advertisePayload = new String( 
    "id: " + xWifi::clientid() 
    + "\nname: " + xWifi::device_name
    + "\ndescription: " +  SYSTEM_DISCOVERY_DEVICE_DESCRIPTION
    + "\ntopics:" + F( 
      // For any module with a control, add it here.  TO-ADD-NEW-SENSOR TO-ADD-NEW-ACTUATOR TO-ADD-NEW-CONTROL
      #ifdef ACTUATOR_LEDBUILTIN_WANT
        ACTUATOR_LEDBUILTIN_ADVERTISEMENT
      #endif
      #ifdef SENSOR_SHT85_WANT
        SENSOR_SHT85_ADVERTISEMENT
      #endif
      #ifdef CONTROL_BLINKEN_WANT
        CONTROL_BLINKEN_ADVERTISEMENT
      #endif
  ));
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

