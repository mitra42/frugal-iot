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

#include <Arduino.h>
#include "system_wifi.h"
#include "system_mqtt.h"  // xMqtt
#include "system_discovery.h"
// TO_ADD_SENSOR TO_ADD_ACTUATOR
#ifdef SENSOR_ANALOG_EXAMPLE_WANT
  #include "sensor_analog_example.h"
#endif
#ifdef SENSOR_BATTERY_WANT
  #include "sensor_battery.h"
#endif
#ifdef CONTROL_DEMO_MQTT_WANT
  #include "control_demo_mqtt.h"
#endif

namespace xDiscovery {

unsigned long nextLoopTime = 0;

//TODO Optimization - should these be String & instead of String *
// projectTopic - gets 30592; 332252 *projectTopic 30584 / 332220
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
// This is a previous idea that clients ask for the fullAdvertise, but since its retained at the broker that isn't needed. 
void inputReceived(String &payload) {
  #ifdef SYSTEM_DISCOVERY_DEBUG
    Serial.print(F("Discovery message receieved:"));
  #endif // SYSTEM_DISCOVERY_DEBUG
  // topic can only be advertiseTopic so no need to test
  if (payload[0] == '?') {
    #ifdef SYSTEM_DISCOVERY_DEBUG
      Serial.println(F("Request for advertisement"));
    #endif
    fullAdvertise();
  }
}
*/

//const char PROGMEM system_discovery_organization_slash[] = SYSTEM_DISCOVERY_ORGANIZATION "/";
#ifdef ESP8266 // Runtime Exception if try and add char[] to String 
  #define idcolon F("id: ")
  #define nlNameColon F("\nname: ")
  //const char PROGMEM xxx[] = ""; //TODO_C++EXPERT - for weird reason requires this and Serial.print(xxx) or get run time exception
#else // ESP32 - can't start a String concat with a F()
  #define idcolon "id: "
  #define nlNameColon F("\nname: ")
#endif

void setup() {
  // This line fails when board 'LOLIN C3 PICO' is chosen
  // projectTopic = new String(F(SYSTEM_DISCOVERY_ORGANIZATION "/") + xWifi::discovery_project + F("/"));
  //Serial.print(xxx); //TODO_C++EXPERT - for weird reason requires this and const char PROGMEM above  or get run time exception
  projectTopic = new String(SYSTEM_DISCOVERY_ORGANIZATION "/" + xWifi::discovery_project + F("/"));
  advertiseTopic = new String(*projectTopic + xWifi::clientid()); // e.g. "dev/Lotus Ponds/esp32-12345"
  topicPrefix = new String(*advertiseTopic + F("/")); // e.g. "dev/Lotus Ponds/esp32-12345/" prefix of most topics
    advertisePayload = new String( 
    idcolon + xWifi::clientid() 
    + nlNameColon + xWifi::device_name
    + F("\ndescription: "
    // Can be overridden in _local.h
    #ifdef SYSTEM_DISCOVERY_DEVICE_DESCRIPTION
      SYSTEM_DISCOVERY_DEVICE_DESCRIPTION
    #else
      #ifdef ESP8266_D1_MINI
        "ESP8266 D1 Mini"
      #else
        #ifdef LOLIN_C3_PICO
          "Lolin C3 Pico"
        #else
          #error undefined board in system_discovery.cpp #TO_ADD_NEW_BOARD
        #endif
      #endif
      // TO-ADD-SENSOR (note space at start of string)
      #ifdef SENSOR_SHT85_WANT
        " SHTxx temp/humidity"
      #endif
      #ifdef SENSOR_DHT_WANT
        " DHT temp/humidity"
      #endif
      // TO-ADD-ACTUATOR
      #ifdef ACTUATOR_RELAY_WANT
        " Relay"
      #endif
    #endif
    // TODO-44 add location: <gsm coords>
    "\ntopics:" 
      // For any module with a control, add it here.  TO_ADD_SENSOR TO_ADD_ACTUATOR TO-ADD-NEW-CONTROL
      #ifdef ACTUATOR_LEDBUILTIN_WANT
        ACTUATOR_LEDBUILTIN_ADVERTISEMENT
      #endif
      #ifdef ACTUATOR_RELAY_WANT
        ACTUATOR_RELAY_ADVERTISEMENT
      #endif
      #ifdef SENSOR_ANALOG_EXAMPLE_WANT
        SENSOR_ANALOG_EXAMPLE_ADVERTISEMENT
      #endif
      #ifdef SENSOR_BATTERY_WANT
        SENSOR_BATTERY_ADVERTISEMENT
      #endif
      #ifdef SENSOR_SHT85_WANT
        SENSOR_SHT85_ADVERTISEMENT
      #endif
      #ifdef SENSOR_DHT_WANT
        SENSOR_DHT_ADVERTISEMENT
      #endif
      #ifdef CONTROL_BLINKEN_WANT
        CONTROL_BLINKEN_ADVERTISEMENT
      #endif
      #ifdef CONTROL_DEMO_MQTT_WANT
        CONTROL_DEMO_MQTT_ADVERTISEMENT
      #endif
    )
  );
  #ifdef SYSTEM_DISCOVERY_DEBUG
    Serial.print(F("topicPrefix=")); Serial.println(*topicPrefix);
  #endif
    fullAdvertise(); // Tell broker what I've got at start (note, intentionally before quickAdvertise) 
    // xMqtt::subscribe(*advertiseTopic, *inputReceived); // Commented out as don't see why need to receive this
}

void loop() {
    if (nextLoopTime <= millis()) {
        quickAdvertise(); // Send info about this node to server (on timer)
        nextLoopTime = millis() + SYSTEM_DISCOVERY_MS;
    }
}

} // namespace xDiscovery
#endif // SYSTEM_DISCOVERY_WANT

