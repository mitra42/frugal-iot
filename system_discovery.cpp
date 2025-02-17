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

#if (!defined(SYSTEM_DISCOVERY_MS) || !defined(SYSTEM_DISCOVERY_ORGANIZATION))
  #error system_discover does not have all requirements in _configuration.h: SYSTEM_DISCOVERY_MS SYSTEM_DISCOVERY_ORGANIZATION
#endif

#include <Arduino.h>
#include "system_wifi.h"
#include "system_mqtt.h"  // xMqtt
#include "system_discovery.h"
// TO_ADD_ACTUATOR
#ifdef ACTUATOR_RELAY_WANT
  #include "actuator_relay.h"
#endif
#ifdef ACTUATOR_LEDBUILTIN_WANT
  #include "actuator_ledbuiltin.h"
#endif
// TO_ADD_SENSOR 
#ifdef SENSOR_ANALOG_EXAMPLE_WANT
  #include "sensor_analog_example.h"
#endif
#ifdef SENSOR_SOIL_WANT
  #include "sensor_soil.h"
#endif
#ifdef SENSOR_BATTERY_WANT
  #include "sensor_battery.h"
#endif
#ifdef SENSOR_DHT_WANT
  #include "sensor_dht.h"
#endif
#ifdef SENSOR_SHT_WANT
  #include "sensor_sht.h"
#endif
// TO_ADD_CONTROL
#ifdef CONTROL_BLINKEN_WANT
  #include "control_blinken.h"
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
      #ifdef SENSOR_SOIL_WANT
        " Soil moisture"
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
      // For any module with a control, add it here.  TO_ADD_SENSOR TO_ADD_ACTUATOR TO_ADD_NEW_CONTROL
      #ifdef ACTUATOR_LEDBUILTIN_WANT
        ACTUATOR_LEDBUILTIN_ADVERTISEMENT
      #endif
      #ifdef ACTUATOR_RELAY_WANT
        ACTUATOR_RELAY_ADVERTISEMENT
      #endif
      #ifdef SENSOR_ANALOG_EXAMPLE_WANT
        SENSOR_ANALOG_EXAMPLE_ADVERTISEMENT
      #endif
      #ifdef SENSOR_SOIL_WANT
        SENSOR_SOIL_ADVERTISEMENT1
        #ifdef SENSOR_SOIL_PIN2
          SENSOR_SOIL_ADVERTISEMENT2
        #endif
        #ifdef SENSOR_SOIL_PIN3
          SENSOR_SOIL_ADVERTISEMENT3
        #endif        
      #endif
      #ifdef SENSOR_BATTERY_WANT
        SENSOR_BATTERY_ADVERTISEMENT
      #endif
      #ifdef SENSOR_SHT_WANT
        SENSOR_SHT_ADVERTISEMENT
      #endif
      #ifdef SENSOR_DHT_WANT
        SENSOR_DHT_ADVERTISEMENT
      #endif
      #ifdef CONTROL_BLINKEN_WANT
        CONTROL_BLINKEN_ADVERTISEMENT
      #endif
      #ifdef SYSTEM_FS_WANT
        system_logger::loggers.advertisementAll();
      #endif
    )
  );
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

