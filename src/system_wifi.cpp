/*
    WiFi Configuration and connection
    Based on the example from https://github.com/Juerd/ESP-WiFiSettings

    Note language support is for text displayed in the portal, others like filenames and debug texts are intentionally left in Engish.
*/

#include "_settings.h"  // Settings for what to include etc - defines LANGUAGE_ALL if LANGUAGE_XX not defined
#ifdef SYSTEM_WIFI_WANT

// #include <Arduino.h>
#if ESP8266
  #include <ESP8266WiFi.h>  // for WiFiClient
#else
  #include <WiFi.h> // This will be platform dependent, will work on ESP32 but most likely want configurration for other chips/boards
#endif
#include "system_wifi.h"
#include "system_mqtt.h"
#ifdef ESP32
  #include <esp_wifi.h> // For esp_wifi_stop / start
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>  // for WiFiClient
#else
  #include <WiFi.h> // This will be platform dependent, will work on ESP32 but most likely want configurration for other chips/boards
#endif
#include "misc.h" // For lprintf
#include "system_frugal.h"

#define ESPFS LittleFS
#include <LittleFS.h>
#include "WiFiSettings.h"  // adapted from https://github.com/Juerd/ESP-WiFiSettings

#ifndef SYSTEM_WIFI_PORTAL_RESTART
  #define SYSTEM_WIFI_PORTAL_RESTART 120000 // How long (ms) portal should wait before restarting - 2 mins probably about right
#endif

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

System_WiFi::System_WiFi()
: System_Base("wifi", "WiFi")
  {}

// Attempt to connect to main network as configured, if succeed save the id and password as a known network
bool System_WiFi::connect1() {
  // Use last stored credentials (if any) to attempt connect to your WiFi access point.
  // store for future use if successfull.
  WiFiSettings.ssid = frugal_iot.fs_LittleFS->slurp("/wifi-ssid");
  String pw = frugal_iot.fs_LittleFS->slurp("/wifi-password");
  //if (WiFiSettings.onConnect) WiFiSettings.onConnect(); // FrugalIot isn't using this currently
  if (WiFiSettings.ssid.length()) {
    if (WiFiSettings.connectInner(WiFiSettings.ssid, pw)) {
      addWiFi(WiFiSettings.ssid, pw);
      //if (WiFiSettings.onSuccess) WiFiSettings.onSuccess(); // FrugalIot not using
      return true;
    }
  }
  return false;
}
bool System_WiFi::scanConnectOneAndAll() {
    //delay(5000); //TODO-125
    #ifdef ESP32
      WiFi.disconnect(true, true);    // reset state so .scanNetworks() works
    #else
        WiFi.disconnect(true);
    #endif
    WiFiSettings.rescan();  // Finishes with print of number of networks
    if (WiFiSettings.ssid.length()) {
      int i;
      for (i = 0; (i < WiFiSettings.num_networks) && (WiFiSettings.ssid != WiFi.SSID(i)); i++) { } // i will be ssid of num_networks if not found 
      if (i == WiFiSettings.num_networks) {
        Serial.print(F("Configured network ")); Serial.print(WiFiSettings.ssid); Serial.print(F(" not found"));
      } else {
        if (connect1()) { // See configured network, try it first
          return true;
        }
      }
    }
    // On failure (or no credentials), scan, and try any that we've successfully connected to before.
    int32_t minRSSI;
    // Running thru strongest networks first
    for (minRSSI = 0; minRSSI > -1000; minRSSI -= 5) {
      int i;
      // Serial.print("RSSI > "); Serial.println(minRSSI);
      for (i = 0; (i < WiFiSettings.num_networks) && (WiFiSettings.ssid != WiFi.SSID(i)); i++) { 
        if ((WiFi.RSSI(i) > minRSSI) && (WiFi.RSSI(i) <= (minRSSI + 5))) {
          String filename = String("/wifi/" + WiFi.SSID(i)) ;
          Serial.print(WiFi.SSID(i)); Serial.print(F(" ")); Serial.print(WiFi.RSSI(i)); Serial.print(F(" "));
          String pw = frugal_iot.fs_LittleFS->slurp(filename);
          if (pw.length()) {
            if (WiFiSettings.connectInner(WiFi.SSID(i), pw)) {
              Serial.print(F("Connected to ")); Serial.println(WiFi.SSID(i));
              return true;
            } 
          } else {
            Serial.println(F("Unknown"));
          }
        }
      } 
    }
    return false;
}
// This is called - blocking - by setup(), but can also be called if discover no longer connected
// Replaces WiFiSettingsClass::connect
bool System_WiFi::connect() {
  WiFiSettings.begin();
  if (scanConnectOneAndAll()) {
    WiFi.setAutoReconnect(true); // Stay connected if lose it - doesnt seem to work at least on TTGO
    return true;
  } else {
    // Tried any networks we know
    // If no successful connection, access point will be started with a captive portal to configure WiFi.
    if (WiFiSettings.onFailure) WiFiSettings.onFailure(); // onFailure sets up portal watchdog 
    WiFiSettings.portal(); // only returns if watchdog connects - if user configures it will reset instead
    return true;
  }
}

// Blocking attempt at reconnecting - can be called by MQTT
void System_WiFi::checkConnected() {
  if (WiFi.status() != WL_CONNECTED) {
    #ifdef SYSTEM_WIFI_DEBUG
      Serial.printf("WiFi status=%d not connected, forcing reconnect\n", WiFi.status());
    #endif
    connect();
  }
}

#ifdef SYSTEM_WIFI_PORTAL_RESTART

// A watchdog on the portal, that will reset after SYSTEM_WIFI_PORTAL_RESTART ms
// Adding ability to reset if wanted wifi appears. 
bool portalWatchdog() {
  // TODO-23 think about sleep and the portal
  // TODO-141 move some of this into the System_WiFi class, just stub it in bare function
  static unsigned long lastWatchdog = frugal_iot.powercontroller->sleepSafeMillis(); // initialized first time this is called
  if (frugal_iot.powercontroller->sleepSafeMillis() > lastWatchdog + (WiFi.softAPgetStationNum() ? SYSTEM_WIFI_PORTAL_RESTART : 15000)) {
    #ifdef SYSTEM_WIFI_DEBUG
      Serial.println(F("WiFiSettings Rescanning"));
    #endif
    // Note this rescan wont be reflected in any any open portal as the HTML generated is static, 
    // but will reflect if user reloads
    if (frugal_iot.wifi->scanConnectOneAndAll()) {
      return true; // Connected - exit portal
    }
    // If noone connected rescan every 15 seconds
    lastWatchdog = frugal_iot.powercontroller->sleepSafeMillis();
  } 
  return false; 
}
#endif // SYSTEM_WIFI_PORTAL_RESTART

String& System_WiFi::clientid() {
  WiFiSettings.begin(); // Ensure WiFi has created variables - at this point any previous ssid and language are now set
  return WiFiSettings.hostname;
}

void System_WiFi::setupLanguages() {
  // TODO-39 need to make sure external for language is set prior to this - get defined from platformio.h and LANGUAGE_ALL
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

void System_WiFi::addWiFi(String ssid, String password) {
  //const String filename = StringF("/wifi/%s",ssid.c_str());
  const String filename("/wifi/" + ssid);
  if (!frugal_iot.fs_LittleFS->spurt(filename,password)) {
    Serial.println("XXX fail to spurt");
  };
}
// Note this is blocking - so order is important, in particular it must complete this before trying mqtt::setup
void System_WiFi::setup() {
  setupLanguages(); // Must come before any calls to WiFiSettings.<anything> 

  // This may be confusing ! 
  // Each line initializes a variable to the existing value, 
  // but override from LittleFS if available, 
  // then adds a line to the WiFi portal that can be used to set the file value, 
  // which will be used after the reboot.

  // Custom configuration variables, these will read configured values if previously set and return default values if not.
  /*
    int integer(String name, [long min, long max,] int init = 0, String label = name);
    String string(String name, [[unsigned int min_length,] unsigned int max_length,] String init = "", String label = name);
    bool checkbox(String name, bool init = false, String label = name);
  */

  frugal_iot.mqtt->hostname = WiFiSettings.string(F("mqtt_host"), 4,40, frugal_iot.mqtt->hostname, T.MqttServer); 
  // TODO-29 turn projet into a dropdown, use an ifdef for the ORGANIZATION in _locals.h not support by ESPWiFi-Settings yet.
  frugal_iot.project = WiFiSettings.string(F("project"), 3,20, frugal_iot.project, T.Project); 
  frugal_iot.device_name = WiFiSettings.string(F("device_name"), 3,20, frugal_iot.device_name, T.DeviceName); 
  #ifdef SYSTEM_WIFI_DEBUG
    Serial.print(F("MQTT host = ")); Serial.println(frugal_iot.mqtt->hostname);
    Serial.print(F("Project = ")); Serial.println(frugal_iot.project);
    Serial.print(F("Device Name = ")); Serial.println(frugal_iot.device_name);
  #endif

  // Cases of connect and portal
  // a: no SSIDs (main or /wifi/) portal run without attempting to connect - never resets
  // b: SSID(s) but connect fails, we have settings, so set a watchdog on portal
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

bool System_WiFi::reconnectWiFi() { // Try hard to reconnect WiFi
  if (WiFi.status() == WL_DISCONNECTED) {
    if (!WiFi.reconnect()) { Serial.println("Failed to reconnect to WiFi"); return 0; }; 
    unsigned long starttime = millis();
    while (WiFi.status() != WL_CONNECTED && ((millis() - starttime) < 3000)) {
        Serial.print(WiFi.status());
        delay(20); // Short delay as expect to be fast
    }
  }
  connect(); // Scan and more complicate connection - even AP portal if fail
  return WiFi.status() == WL_CONNECTED;
}

#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32
bool System_WiFi::prepareForLightSleep() {
   return (esp_wifi_stop() == ESP_OK); // Suggested to reduce dropping WiFi connection
}
#endif

#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32
bool System_WiFi::recoverFromLightSleep() {
  if (esp_wifi_start() != ESP_OK) {
    Serial.println(F("Failed to restart esp_wifi"));
    return 0;
  }
   // Spin till its started
  while (WiFi.status() == WL_NO_SHIELD) {
     /*Serial.print("w");*/ 
     delay(100); 
  } 
  return reconnectWiFi(); // Quick connect if possible - otherwise scan and connect
}
#endif // ESP32

#endif // SYSTEM_WIFI_WANT