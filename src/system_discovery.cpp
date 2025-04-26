/* Manage MQTT higher level 
   Advertise in a way that allows a client to discover the nodes from knowing the project
 
   Periodically (SYSTEM_DISCOVERY_MS) send node name on project  e.g  "dev/Lotus/" = "node1"
  At startup send a YAML string that describes this node and all sensors actuators

  Required SYSTEM_DISCOVERY_MS 
  Required SYSTEM_DISCOVERY_ORGANIZATION
  Optional SYSTEM_DISCOVERY_DEBUG
  Optional *_WANT and *ADVERTISEMENT for each sensor and actuator

*/

#include "_settings.h"

#ifdef SYSTEM_DISCOVERY_WANT // Until have BLE, no WIFI means local only

#if !defined(SYSTEM_DISCOVERY_ORGANIZATION)
  #error system_discover does not have all requirements in _locals.h: SYSTEM_DISCOVERY_ORGANIZATION
#endif

#ifndef SYSTEM_DISCOVERY_MS
  #define SYSTEM_DISCOVERY_MS (30000) // quick discovery every 30 seconds
#endif

#include <Arduino.h>
#include "system_wifi.h"
#include "system_mqtt.h"  // xMqtt
#include "system_discovery.h"
#ifdef ACTUATOR_WANT
  #include "actuator.h"
#endif
#ifdef SENSOR_WANT
  #include "sensor.h"
#endif
#ifdef CONTROL_WANT
  #include "control.h"
#endif

namespace xDiscovery {

unsigned long nextLoopTime = 0;

//TODO Optimization - should these be String & instead of String *
// projectTopic - gets 30592; 332252 *projectTopic 30584 / 332220
String *projectTopic;
String *advertiseTopic;
String *topicPrefix;
#ifdef SYSTEM_OTA_WANT
  String *otaKey;
#endif

String *advertisePayload;
void quickAdvertise() {
    Mqtt->messageSend(*projectTopic,  xWifi::clientid(), false, 0); // Don't RETAIN as other nodes also broadcasting to same topic
}


//const char PROGMEM system_discovery_organization_slash[] = SYSTEM_DISCOVERY_ORGANIZATION "/";
#ifdef ESP8266 // Runtime Exception if try and add char[] to String 
  #define idcolon F("id: ")
  #define nlNameColon F("\nname: ")
  //const char PROGMEM xxx[] = ""; //TODO_C++EXPERT - for weird reason requires this and Serial.print(xxx) or get run time exception
#else // ESP32 - can't start a String concat with a F()
  #define idcolon "id: "
  #define nlNameColon F("\nname: ")
#endif

void fullAdvertise() {
  // Note - this is intentionally not a global string as it can be quite big, better to create, send an free up
  String* advertisePayload = new String( 
    idcolon + xWifi::clientid() 
    + nlNameColon + xWifi::device_name
    + F("\ndescription: "
    // Can be overridden in _local.h
    #ifdef SYSTEM_DISCOVERY_DEVICE_DESCRIPTION
      SYSTEM_DISCOVERY_DEVICE_DESCRIPTION
    #else
      //TO_ADD_BOARD - only used if SYSTEM_DISCOVERY_DEVICE_DESCRIPTION undefined and displayed in UX.
      #ifdef ESP8266_D1
        "ESP8266 D1"
      #elif defined(LOLIN_C3_PICO)
        "Lolin C3 Pico"
      #else
        #error undefined board in system_discovery.cpp #TO_ADD_NEW_BOARD
      #endif
      // TO_ADD_SENSOR (note space at start of string)
      #ifdef SENSOR_SHT_WANT
        " SHTxx temp/humidity"
      #endif
      #ifdef SENSOR_DHT_WANT
        " DHT temp/humidity"
      #endif
      // TO_ADD_ACTUATOR
      #ifdef ACTUATOR_RELAY_WANT
        " Relay"
      #endif
    #endif
    #ifdef SYSTEM_OTA_WANT
      "\nota: " SYSTEM_OTA_KEY 
    #endif
    // TODO-44 add location: <gsm coords>
    "\ntopics:" 
    )
  );
  #ifdef ACTUATOR_WANT
    *advertisePayload += (Actuator::advertisementAll());
  #endif
  #ifdef SENSOR_WANT
    *advertisePayload += (Sensor::advertisementAll());
  #endif
  #ifdef CONTROL_WANT
    *advertisePayload += (Control::advertisementAll());
  #endif
  Mqtt->messageSend(*advertiseTopic, *advertisePayload, true, 1);
}

void setup() {
  // This line fails when board 'LOLIN C3 PICO' is chosen
  // projectTopic = new String(F(SYSTEM_DISCOVERY_ORGANIZATION "/") + xWifi::discovery_project + F("/"));
  //Serial.print(xxx); //TODO_C++EXPERT - for weird reason requires this and const char PROGMEM above  or get run time exception
  projectTopic = new String(SYSTEM_DISCOVERY_ORGANIZATION "/" + xWifi::discovery_project );
  advertiseTopic = new String(*projectTopic + F("/") + xWifi::clientid()); // e.g. "dev/lotus/esp32-12345"
  topicPrefix = new String(*advertiseTopic + F("/")); // e.g. "dev/lotus/esp32-12345/" prefix of most topics
  #ifdef SYSTEM_DISCOVERY_DEBUG
    Serial.print(F("topicPrefix=")); Serial.println(*topicPrefix);
  #endif
}

void loop() {
    if (nextLoopTime <= millis()) {
        quickAdvertise(); // Send info about this node to server (on timer)
        nextLoopTime = millis() + SYSTEM_DISCOVERY_MS;
    }
}

} // namespace xDiscovery
#endif // SYSTEM_DISCOVERY_WANT

