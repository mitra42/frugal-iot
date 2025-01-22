/* OTA client
* 
* Please see https://github.com/mitra42/frugal-iot/blob/main/docs/ota.md and 
* and https://github.com/mitra42/frugal-iot/issues/37
* 
* This is based on  https://github.com/N4rcissist/OtaHelper/blob/main/src/OtaHelper.h
* Note - Couldn't I couldn't get other example code to work 
* ESP32 Based on the example in https://github.com/espressif/arduino-esp32/blob/master/libraries/Update/examples/HTTPS_OTA_Update/HTTPS_OTA_Update.ino
* ESP8266 based on example in https://www.instructables.com/Set-Up-an-ESP8266-Automatic-Update-Server/
* 
* Configuration
* Required: SYSTEM_OTA_KEY short string (defined by "org") for different boards
* Optional: SYSTEM_OTA_DEBUG SYSTEM_OTA_MS SYSTEM_OTA_SERVERPORTPATH 
* 
*/

#include "_settings.h"
#ifdef SYSTEM_OTA_WANT

#ifndef ESP8266
  #error OTA is currently only defined for ESP8266
#endif

#include <Arduino.h>
#include "system_discovery.h"
#include "system_ota.h"

#ifdef ESP8266
#include <ESP8266httpUpdate.h>
#endif

#ifndef SYSTEM_OTA_MS
  // By default, check for updates once an hour
  #define SYSTEM_OTA_MS 3600000
#endif // SYSTEM_OTA_MS
#ifndef SYSTEM_OTA_VERSION
  #define SYSTEM_OTA_VERSION "0.0.0" // We dont use versions, we are using MD5
#endif

#ifndef SYSTEM_OTA_SERVERPORTPATH
  #define SYSTEM_OTA_SERVERPORTPATH "http://naturalinnovation.org:8080/ota_update/" // Note trailing slash
#endif

namespace xOta {

WiFiClient net; // Assumes system_wifi has already connected to access point - note this will not run if Wifi fails to connect and goes to portal mode
unsigned long nextLoopTime = SYSTEM_OTA_MS; // Dont activate on first loop - as happens in Setup

#ifdef SYSTEM_OTA_DEBUG

  void update_started() {
    Serial.println(F("HTTP update process started"));
  }

  void update_finished() {
    Serial.println(F("HTTP update process finished\n"));
  }

  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-parameter"
  void update_progress(int cur, int total) {
  #pragma GCC diagnostic pop
    Serial.print(F("."));
  }

  void update_error(int err) {
    Serial.printf("HTTP update fatal error code %d\n", err);
  }
#endif // SYSTEM_OTA_DEBUG

void updateAndReboot(bool sketch) {
  ESPhttpUpdate.rebootOnUpdate(false); // Dont automatically reboot - do it under control below
  #ifdef SYSTEM_OTA_DEBUG
    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW); // Note that this is subject to usual issues with LED_BUILTIN being wrong on some boards (see actuator_ledbuiltin)
    ESPhttpUpdate.onStart(update_started);
    ESPhttpUpdate.onEnd(update_finished);
    ESPhttpUpdate.onProgress(update_progress);
    ESPhttpUpdate.onError(update_error);
  #endif

  if (sketch) {
    // TODO-37 parameterize host:port 
    // Note there is no correlation between the path here, and where its stored on the server which also pays attention to dev/project/node
    String *url = new String(F(SYSTEM_OTA_SERVERPORTPATH) + *xDiscovery::topicPrefix + SYSTEM_OTA_KEY);
    #ifdef SYSTEM_OTA_DEBUG
      Serial.print("Attempt OTA from:"); Serial.println(*url);
    #endif
    t_httpUpdate_return ret = ESPhttpUpdate.update(net, *url, SYSTEM_OTA_VERSION);
    if (ret == HTTP_UPDATE_OK) {
      #ifdef SYSTEM_OTA_DEBUG
        Serial.println("HTTP_UPDATE_OK. Rebooting to install new flash");
      #endif
      delay(1000); // Wait a second and restart (not sure if there is a reason for this delay)
      ESP.restart();
    #ifdef SYSTEM_OTA_DEBUG
      } else if (ret == HTTP_UPDATE_NO_UPDATES) {
          Serial.print("No need to update");
      } else {
        Serial.println("Update failed");
    #endif // SYSTEM_OTA_DEBUG
    }
  } else {
    Serial.println("TODO-37 not defined for SPIFFS yet");
  }
}

void setup() {
  updateAndReboot(true); // Note this must be after WiFi has connected  and ideally before MQTT or Discovery
}

void loop() {
  // Note wont operate on first loop (see initialization of nextLoopTime)
  if (nextLoopTime <= millis() ) {
    updateAndReboot(true); // Note this must be after WiFi has connected  and ideally before MQTT or Discovery
    nextLoopTime = millis() + SYSTEM_OTA_MS;
  }
}

} // namespace xOta

#endif // SYSTEM_OTA_WANT
