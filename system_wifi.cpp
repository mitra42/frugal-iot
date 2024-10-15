/*
    Wifi Configuration and connection
    Based on the example from https://github.com/Juerd/ESP-WiFiSettings
*/

// TODO - remove extra wifi in system_mqtt and make sure it can find this

#include "_settings.h"  // Settings for what to include etc
#ifdef SYSTEM_WIFI_WANT

// #include <Arduino.h>
#if ESP8266
#include <ESP8266WiFi.h>  // for WiFiClient
#else
#include <WiFi.h> // This will be platform dependent, will work on ESP32 but most likely want configurration for other chips/boards
#endif
// #include "_common.h"
#include "system_wifi.h"


// TODO find a way to store in eeprom rather than SPIFFS. 
#ifdef ESP32
  #include <SPIFFS.h>
#elif ESP8266
  #include <LittleFS.h>
#else
    #error "This example only supports ESP32 and ESP8266"
#endif 
#include <WiFiSettings.h>  // https://github.com/Juerd/ESP-WiFiSettings

namespace xWifi {

String mqtt_host;

// This is called - blocking - by xWiFi.setup, but can also be called if discover no longer connected
void connect() {
    // Use stored credentials to connect to your WiFi access point.
    // If no credentials are stored or if the access point is out of reach,
    // an access point will be started with a captive portal to configure WiFi.
    WiFiSettings.connect();
    // Serial.println("XXX-22a connect exited gracefully without reset")
}

// Note this is blocking - so order is important, in particular it must complete this before trying xMqtt::setup
void setup() {
  #ifdef ESP32
    SPIFFS.begin(true); // Will format on the first run after failing to mount
  #elif ESP8266
    LittleFS.begin();  
  #else
    #error "This example only supports ESP32 and ESP8266"
  #endif

  // Custom configuration variables, these will read configured values if previously set and return default values if not.
  mqtt_host = WiFiSettings.string("mqtt_host", 4,40, SYSTEM_MQTT_SERVER, "MQTT Host"); 
  
  // TODO-22a WiFiSettings.onPortalWaitLoop may be the hook to recheck if WiFi connected and if so restart or maybe exit? 
  // TODO-22a Or maybe just countdown and clear some flag
  // WiFiSettings.onPortalWaitLoop = []() { 
  //  Serial.println("XXX-22 onPortalWaitLoop");
  // };
  connect();
}

/*
void loop() { } //TODO-22 maybe try and reconnect here ?
*/
} // namespace xWifi
#endif SYSTEM_WIFI_WANT