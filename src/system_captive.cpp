/* Frugal IoT - captive portal
 * 
 * This is a trivial captive portal, that allows for configuration etc. 
 * 
 * Its main use is for setting the WiFi settings.
 * 
 * BUT .... this is extensible, at some point we might start doing any or all of the following.
 * - displaying current data
 * - informing about version, configured devices and their parameters
 * - downloading saved data files
 * 
 * Its based on the example that comes with ESPAsyncWebServer 
 * https://github.com/ESP32Async/ESPAsyncWebServer/blob/main/examples/CaptivePortal/CaptivePortal.ino
 */

// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2025 Hristo Gochkov, Mathieu Carbou, Emil Muratov

#include <DNSServer.h>
#if defined(ESP32) || defined(LIBRETINY)
#include <AsyncTCP.h>
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#elif defined(TARGET_RP2040) || defined(TARGET_RP2350) || defined(PICO_RP2040) || defined(PICO_RP2350)
// Untested - these just come from the example
//#include <RPAsyncTCP.h>
//#include <WiFi.h>
#endif

#include "ESPAsyncWebServer.h"
#include "system_captive.h"

static DNSServer dnsServer;
static AsyncWebServer server(80);

class CaptiveRequestHandler : public AsyncWebHandler {
 public:
  bool canHandle(__unused AsyncWebServerRequest *request) const override {
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->print("<!DOCTYPE html><html><head><title>Captive Portal</title></head><body>");
    response->print("<p>This is our captive portal front page.</p>");
    response->printf("<p>You were trying to reach: http://%s%s</p>", request->host().c_str(), request->url().c_str());
#if SOC_WIFI_SUPPORTED || CONFIG_ESP_WIFI_REMOTE_ENABLED || LT_ARD_HAS_WIFI
    response->printf("<p>Try opening <a href='http://%s'>this link</a> instead</p>", WiFi.softAPIP().toString().c_str());
#endif
    response->print("</body></html>");
    request->send(response);
  }
};


System_Captive::System_Captive()
: System_Base("captive", "Captive") {}

void System_Captive::setup() {
  Serial.println("Configuring access point...");

  #if SOC_WIFI_SUPPORTED || CONFIG_ESP_WIFI_REMOTE_ENABLED || LT_ARD_HAS_WIFI
    if (!WiFi.softAP("esp-captive")) {
      Serial.println("Soft AP creation failed.");
      while (1);
    }
    dnsServer.start(53, "*", WiFi.softAPIP());
  #endif
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);  // only when requested from AP
  // more handlers...
  server.begin();
}

void System_Captive::frequently() {
  dnsServer.processNextRequest();
}


#ifdef NOTINCORPORATEDYET

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

  frugal_iot.mqtt->hostname = WiFiSettings.string(F("mqtt/hostname"), 4,40, frugal_iot.mqtt->hostname, T.MqttServer); 
  // TODO-29 turn projet into a dropdown, use an ifdef for the ORGANIZATION in _locals.h not support by ESPWiFi-Settings yet.
  frugal_iot.project = WiFiSettings.string(F("frugal_iot/project"), 3,20, frugal_iot.project, T.Project); 
  frugal_iot.device_name = WiFiSettings.string(F("frugal_iot/device_name"), 3,20, frugal_iot.device_name, T.DeviceName); 
  #ifdef SYSTEM_WIFI_DEBUG
    Serial.print(F("MQTT host = ")); Serial.println(frugal_iot.mqtt->hostname);
    Serial.print(F("Project = ")); Serial.println(frugal_iot.project);
    Serial.print(F("Device Name = ")); Serial.println(frugal_iot.device_name);
  #endif


// These are the language texts to use 
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

#endif //NOTINCORPORATEDYET