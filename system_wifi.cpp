/*
    Wifi Configuration and connection
    Based on the example from https://github.com/Juerd/ESP-WiFiSettings

    Note language support is for text displayed in the portal, others like filenames and debug texts are intentionally left in Engish.
*/

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
#include "src/lib/WiFiSettings/WiFiSettings.h"  // https://github.com/Juerd/ESP-WiFiSettings

namespace xWifi {

struct Texts {
    const __FlashStringHelper
      *MqttServer,
      *DeviceName,
      *Project
    ;
    /*
    const char
        *init
    ;
    */
};
Texts T;

String mqtt_host;
String discovery_project;
String device_name;

// This is called - blocking - by xWiFi.setup, but can also be called if discover no longer connected
bool connect() {
  // Replaces WiFiSettingsClass::connect
    WiFiSettings.begin();
    // Use last stored credentials (if any) to attempt connect to your WiFi access point.
    WiFiSettings.ssid = slurp("/wifi-ssid");
    String pw = slurp("/wifi-password");
    if (WiFiSettings.onConnect) WiFiSettings.onConnect();
    if (WiFiSettings.ssid.length()) {
      if (WiFiSettings.connectInner(WiFiSettings.ssid, pw)) {
        String filename = String("/wifi/" + WiFiSettings.ssid);
        Serial.print("Saving password in"); Serial.println(filename);
        spurt(filename, pw ); // Save password as a successfully connected network
        if (WiFiSettings.onSuccess) WiFiSettings.onSuccess();
        return true;
      }
    }
    //delay(5000); //TODO-125
    #ifdef ESP32
      WiFi.disconnect(true, true);    // reset state so .scanNetworks() works
    #else
        WiFi.disconnect(true);
    #endif

    // On failure (or no credentials), scan, and try any that we've successfully connected to before.
    WiFiSettings.rescan();  // Finishes with print of number of networks
    int32_t minRSSI;
    int i;
    // Running thru strongest networks first
    for (minRSSI = 0; minRSSI > -1000; minRSSI -= 5) {
      // Serial.print("RSSI > "); Serial.println(minRSSI);
      for (i = 0; (i < WiFiSettings.num_networks) && (WiFiSettings.ssid != WiFi.SSID(i)); i++) { 
        if ((WiFi.RSSI(i) > minRSSI) && (WiFi.RSSI(i) <= (minRSSI + 5))) {
          String filename = String("/wifi/" + WiFi.SSID(i)) ;
          Serial.print(WiFi.SSID(i)); Serial.print(F(" ")); Serial.println(WiFi.RSSI(i));
          String pw = slurp(filename);
          if (pw.length()) {
            Serial.println("Trying");
            if (WiFiSettings.connectInner(WiFi.SSID(i), pw)) {
              Serial.print("Connected to"); Serial.println(WiFi.SSID(i));
              return true;
            } 
          } else {
            Serial.println("Unknown");
          }
        }
      } 
    }
    // Tried any networks we know
    // If no successful connection, access point will be started with a captive portal to configure WiFi.
    if (WiFiSettings.onFailure) WiFiSettings.onFailure();
    WiFiSettings.portal();
    return false;
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
/*
#ifdef SYSTEM_WIFI_SSID
bool spurt(const String& fn, const String& content) {
    File f = ESPFS.open(fn, "w");
    if (!f) return false;
    auto w = f.print(content);
    f.close();
    return w == content.length();
}
#endif
*/
#ifdef SYSTEM_WIFI_PORTAL_RESTART

// A watchdog on the portal, that will reset after SYSTEM_WIFI_PORTAL_RESTART ms
// Adding ability to reset if wanted wifi appears. 
void portalWatchdog() {
  static unsigned long OPWLrestart = millis() + SYSTEM_WIFI_PORTAL_RESTART; // initialized first time this is called
  if (OPWLrestart < millis()) {
      #ifdef SYSTEM_WIFI_DEBUG
        Serial.println(F("WiFiSettings Rescanning"));
      #endif
    // Note this rescan wont be reflected in a any open portal as the HTML generated is static
    WiFiSettings.rescan();
    int i;
    for (i = 0; (i < WiFiSettings.num_networks) && (WiFiSettings.ssid != WiFi.SSID(i)); i++) { } // i will be ssid of num_networks if not found 
    if (i != WiFiSettings.num_networks) { // we found it
      #ifdef SYSTEM_WIFI_DEBUG
        Serial.print(F("WiFiSettings portal timed out and restarting cos now see")); Serial.println(WiFiSettings.ssid);
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
  WiFiSettings.begin(); // Ensure WiFi has created variables - at this point any previous ssid and language are now set
  return WiFiSettings.hostname;
}

void setupLanguages() {
  // TODO-39 need to make sure external for language is set prior to this - get defined from _local.h and LANGUAGE_ALL from configuration.h
  #ifdef LANGUAGE_DEFAULT
    WiFiSettings.language = LANGUAGE_DEFAULT; // This must happen BEFORE WiFiSettings.begin().
  #endif
  WiFiSettings.begin(); // WiFi has created variables - at this point any previous ssid and language are now set
  Serial.print(F("Language = ")); Serial.println(WiFiSettings.language);
  #if defined LANGUAGE_EN || defined LANGUAGE_ALL
    if (WiFiSettings.language == "en") {
      T.MqttServer = F("MQTT server");
      T.DeviceName = F("Device name");
      T.Project = F("Project");
    } 
  #endif
  #if defined LANGUAGE_DE || defined LANGUAGE_ALL
    // German settings all machine translated - confirmation from native German speaker, or better translations welcome
    if (WiFiSettings.language == "de") {
      T.MqttServer = F("MQTT server");
      T.DeviceName = F("Gerätename");
      T.Project = F("Projekt");
    }
  #endif
  #if defined LANGUAGE_NL || defined LANGUAGE_ALL
    // Dutch settings all machine translated - confirmation from native Dutch speaker, or better translations welcome
    if (WiFiSettings.language == "de") {
      T.MqttServer = F("MQTT server");
      T.DeviceName = F("Apparaatnaam");
      T.Project = F("Project");
    }
  #endif
  #if defined LANGUAGE_ID || defined LANGUAGE_ALL
    // Indonesian settings all machine translated - confirmation from native Bahasa speaker, or better translations welcome
    if (WiFiSettings.language == "id") {
      T.MqttServer = F("MQTT server");
      T.DeviceName = F("Nama Perangkat");
      T.Project = F("Proyek");
    }
  #endif
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
  
  setupLanguages(); // Must come before any calls to WiFiSettings.<anything> 

  #ifdef SYSTEM_WIFI_SSID
    Serial.println(F("Overriding WiFi SSID / Password for development"));
    spurt(F("/wifi-ssid"), F(SYSTEM_WIFI_SSID));
    spurt(F("/wifi-password"), F(SYSTEM_WIFI_PASSWORD));
  #endif // SYSTEM_WIFI_SSID
  #ifdef SYSTEM_WIFI_SSID_1
    spurt(F("/wifi/"  SYSTEM_WIFI_SSID_1), F(SYSTEM_WIFI_PASSWORD_1));
  #endif // SYSTEM_WIFI_SSID
  #ifdef SYSTEM_WIFI_SSID_2
    spurt(F("/wifi/"  SYSTEM_WIFI_SSID_2), F(SYSTEM_WIFI_PASSWORD_2));
  #endif
  #ifdef SYSTEM_WIFI_SSID_3
    spurt(F("/wifi/"  SYSTEM_WIFI_SSID_3), F(SYSTEM_WIFI_PASSWORD_3));
  #endif
  #ifdef SYSTEM_WIFI_SSID_4
    spurt(F("/wifi/"  SYSTEM_WIFI_SSID_4), F(SYSTEM_WIFI_PASSWORD_4));
  #endif
  #ifdef SYSTEM_WIFI_SSID_5
    spurt(F("/wifi/"  SYSTEM_WIFI_SSID_5), F(SYSTEM_WIFI_PASSWORD_5));
  #endif

  #ifndef SYSTEM_WIFI_DEVICE
    #define SYSTEM_WIFI_DEVICE "device"
  #endif
  #ifndef SYSTEM_WIFI_PROJECT
    #define SYSTEM_WIFI_PROJECT "project"
  #endif

  // Custom configuration variables, these will read configured values if previously set and return default values if not.
  /*
    int integer(String name, [long min, long max,] int init = 0, String label = name);
    String string(String name, [[unsigned int min_length,] unsigned int max_length,] String init = "", String label = name);
    bool checkbox(String name, bool init = false, String label = name);
  */

  mqtt_host = WiFiSettings.string(F("mqtt_host"), 4,40, F(SYSTEM_MQTT_SERVER), T.MqttServer); 
  // TODO-29 turn discovery_project into a dropdown, use an ifdef for the ORGANIZATION in configuration.h not support by ESPWifi-Settings yet.
  discovery_project = WiFiSettings.string(F("discovery_project"), 3,20, F(SYSTEM_WIFI_PROJECT), T.Project); 
  device_name = WiFiSettings.string(F("device_name"), 3,20, F(SYSTEM_WIFI_DEVICE), T.DeviceName); 
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
        Serial.print(F("Setting portal watchdog for ms"));
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