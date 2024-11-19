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
#include "system_wifi.h"


// TODO find a way to store in eeprom rather than SPIFFS. 
#ifdef ESP32
  #define ESPFS SPIFFS
  #include <SPIFFS.h>
#elif ESP8266
  #define ESPFS LittleFS
  #include <LittleFS.h>
#else
    #error "This example only supports ESP32 and ESP8266"
#endif 
#include <WiFiSettings.h>  // https://github.com/Juerd/ESP-WiFiSettings

namespace xWifi {

String mqtt_host;
String discovery_project;
String device_name;

// This is called - blocking - by xWiFi.setup, but can also be called if discover no longer connected
void connect() {
    // Use stored credentials to connect to your WiFi access point.
    // If no credentials are stored or if the access point is out of reach,
    // an access point will be started with a captive portal to configure WiFi.
    WiFiSettings.connect();
    // If WiFi connected, returns true, if WiFi fails then puts up portal and never returns - portal initiates reset 
    // Serial.println(F("XXX-22a connect exited gracefully without reset"))
}
// Blocking attempt at reconnecting - can be called by MQTT
void checkConnected() {
  if (WiFi.status() != WL_CONNECTED) {
    #ifdef SYSTEM_WIFI_DEBUG
      Serial.println(F("WiFi not connected, forcing reconnect"));
    #endif
    connect();
  }
}
#ifdef SYSTEM_WIFI_SSID
bool spurt(const String& fn, const String& content) {
    File f = ESPFS.open(fn, "w");
    if (!f) return false;
    auto w = f.print(content);
    f.close();
    return w == content.length();
}
#endif

#ifdef SYSTEM_WIFI_PORTAL_RESTART

// Copied from WiFiSettings - TODO would be nice to export from there
String slurp(const String& fn) {
    File f = ESPFS.open(fn, "r");
    String r = f.readString();
    f.close();
    return r;
}

// A watchdog on the portal, that will reset after SYSTEM_WIFI_PORTAL_RESTART ms
// Adding ability to reset if wanted wifi appears. 
void portalWatchdog() {
  static unsigned long OPWLrestart = millis() + SYSTEM_WIFI_PORTAL_RESTART; // initialized first time this is called
  static String current = slurp("/wifi-ssid"); // Get from WiFiSettings - only changes after Save which usually leads to restart
  if (OPWLrestart < millis()) {
      #ifdef SYSTEM_WIFI_DEBUG
        Serial.println(F("WiFiSettings Rescanning"));
      #endif
    const int num_networks = WiFi.scanNetworks();
    int i;
    for (i = 0; (i < num_networks) && (current != WiFi.SSID(i)); i++) { } // i will be ssid o num_networks if not found 
    if (i != num_networks) { // we found it
      #ifdef SYSTEM_WIFI_DEBUG
        Serial.println(F("WiFiSettings portal timed out - found Wifi so restarting"));
      #endif
      if (WiFiSettings.onRestart) { WiFiSettings.onRestart(); } // We aren't setting it here so should do nothing
      ESP.restart();
    } else {
      OPWLrestart = millis() + SYSTEM_WIFI_PORTAL_RESTART;
    }
  }
}
#endif // SYSTEM_WIFI_PORTAL_RESTART

String &clientid() {
  WiFiSettings.begin(); // Ensure WiFi has created variables
  return WiFiSettings.hostname;
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

  // TODO-61 add default SSID & PW if defined in _local.h
  #ifdef SYSTEM_WIFI_SSID
    Serial.println("Overriding WiFi SSID / Password for dev");
    spurt("/wifi-ssid", SYSTEM_WIFI_SSID);
    spurt("/wifi-password", SYSTEM_WIFI_PASSWORD);
  #endif // SYSTEM_WIFI_SSID

  // Custom configuration variables, these will read configured values if previously set and return default values if not.
  /*
    int integer(String name, [long min, long max,] int init = 0, String label = name);
    String string(String name, [[unsigned int min_length,] unsigned int max_length,] String init = "", String label = name);
    bool checkbox(String name, bool init = false, String label = name);
  */

  #ifndef SYSTEM_WIFI_DEVICE
    #define SYSTEM_WIFI_DEVICE "device"
  #endif
  #ifndef SYSTEM_WIFI_PROJECT
    #define SYSTEM_WIFI_PROJECT "project"]
  #endif
  mqtt_host = WiFiSettings.string(F("mqtt_host"), 4,40, F(SYSTEM_MQTT_SERVER), F("MQTT Host")); 
  // TODO-29 turn discovery_project into a dropdown, use an ifdef for the ORGANIZATION in configuration.h not support by ESPWifi-Settings yet.
  discovery_project = WiFiSettings.string(F("discovery_project"), 3,20, F(SYSTEM_WIFI_PROJECT), F("Project")); 
  device_name = WiFiSettings.string(F("device_name"), 3,20, F(SYSTEM_WIFI_DEVICE), F("Device Name")); 
  #ifdef SYSTEM_WIFI_DEBUG
  Serial.print(F("MQTT host = ")); Serial.println(mqtt_host);
  Serial.print(F("Project = ")); Serial.println(discovery_project);
  Serial.print(F("Device Name = ")); Serial.println(device_name);
  #endif

  // Cases of connect and portal
  // a: no SSID; portal run without attempting to connect - never resets
  // b: SSID but connect fails, we have settings, so set a watchdog on portal
  // c: Something (e.g. MQTT) calls checkConnected, which calls connect - SSID will be set, so should attempt, and if fail - do portal with watchdog
  #ifdef SYSTEM_WIFI_PORTAL_RESTART
    WiFiSettings.onFailure = []() {
      #ifdef SYSTEM_WIFI_DEBUG
        Serial.print(F("Setting portal watchdog for "));
        Serial.println(SYSTEM_WIFI_PORTAL_RESTART);
      #endif
      WiFiSettings.onPortalWaitLoop = portalWatchdog;
    };
  #endif // SYSTEM_WIFI_PORTAL_RESTART
  #ifdef SYSTEM_WIFI_DEBUG
    Serial.print(F("WiFi hostname="));
    Serial.println(clientid());
  #endif // SYSTEM_WIFI_DEBUG
  connect();
}

} // namespace xWifi
#endif // SYSTEM_WIFI_WANT