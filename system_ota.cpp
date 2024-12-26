/* OTA client
* Couldn't get these to work
* ESP32 Based on the example in https://github.com/espressif/arduino-esp32/blob/master/libraries/Update/examples/HTTPS_OTA_Update/HTTPS_OTA_Update.ino
* ESP8266 based on example in https://www.instructables.com/Set-Up-an-ESP8266-Automatic-Update-Server/
* 
* Working from https://github.com/N4rcissist/OtaHelper/blob/main/src/OtaHelper.h
* 
* Configuration
* Required: SYSTEM_OTA_MS
* Optional: SYSTEM_OTA_DEBUG
* 
*/
// TODO-37 carefully review whole file for bits copied from example but not needed

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

#define SYSTEM_OTA_DEBUG

namespace xOta {

WiFiClient net;

#ifdef SYSTEM_OTA_DEBUG

  void update_started() {
    Serial.println("CALLBACK:  HTTP update process started");
  }

  void update_finished() {
    Serial.println("CALLBACK:  HTTP update process finished");
  }

  void update_progress(int cur, int total) {
    Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
  }

  void update_error(int err) {
    Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
  }
#endif // SYSTEM_OTA_DEBUG

void updateAndReboot(bool sketch) {
  ESPhttpUpdate.rebootOnUpdate(false);
  #ifdef SYSTEM_OTA_DEBUG
    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
    ESPhttpUpdate.onStart(update_started);
    ESPhttpUpdate.onEnd(update_finished);
    ESPhttpUpdate.onProgress(update_progress);
    ESPhttpUpdate.onError(update_error);
  #endif

  if (sketch) {
    // TODO-37 parameterize host:port 
    String *url = new String(F("http://192.168.1.178:8080/ota_update/") + *xDiscovery::topicPrefix + *xDiscovery::otaKey);
    Serial.print("Attempt OTA from:"); Serial.println(*url);
    t_httpUpdate_return ret = ESPhttpUpdate.update(net, *url, SYSTEM_OTA_VERSION);
    Serial.print("Update return="); Serial.println(ret);
    if (ret == HTTP_UPDATE_OK) {
      Serial.println("HTTP_UPDATE_OK. Reboot");
      delay(1000); // Wait a second and restart
      //ESP.restart(); //TODO-37 enable restart once working
    }
  } //TODO do spiffs 
}

void setup() {
  updateAndReboot(true);
}
void loop() {
  // TODO-37 setup a timer and retry reboot periodically (e.g. once an hour)
}

} // namespace xOta

#endif // SYSTEM_OTA_WANT
