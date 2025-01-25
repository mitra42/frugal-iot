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
* Notes for ESP32
* https://github.com/espressif/arduino-esp32/blob/master/libraries/Update/examples/HTTPS_OTA_Update/HTTPS_OTA_Update.ino 
* uses https://github.com/espressif/arduino-esp32/tree/master/libraries/Update/src/HttpsOTAUpdate.h and .cpp
*/

#include "_settings.h"
#ifdef SYSTEM_OTA_WANT

#if ! (defined(ESP8266) || defined(ESP32))
  #error OTA is currently only defined for ESP8266 and ESP32
#endif

#include <Arduino.h>
#include "system_discovery.h"
#include "system_wifi.h"
#include "system_ota.h"
#ifdef ESP8266
#include <ESP8266httpUpdate.h>
#endif
#if ESP8266 // Note ESP8266 and ESP32 are defined for respective chips - unclear if anything like that for other Arduinos
  #include <ESP8266WiFi.h>  // for WiFiClient
#else
  #include <WiFi.h> // This will be platform dependent, will work on ESP32 but most likely want configurration for other chips/boards
#endif


#ifndef SYSTEM_OTA_MS
  // By default, check for updates once an hour
  #define SYSTEM_OTA_MS 3600000
#endif // SYSTEM_OTA_MS
#ifndef SYSTEM_OTA_VERSION
  #define SYSTEM_OTA_VERSION "0.0.0" // We dont use versions, we are using MD5
#endif

#ifndef SYSTEM_OTA_SERVERPORTPATH
  #define SYSTEM_OTA_SERVERPORTPATH "https://frugaliot.naturalinnovation.org/ota_update/" // Note trailing slash
#endif

#ifdef ESP32 //See https://github.com/espressif/arduino-esp32/blob/master/libraries/Update/examples/HTTPS_OTA_Update/HTTPS_OTA_Update.ino
  #include "HttpsOTAUpdate.h"

//TODO-37 this may need to be root as changes every 3 months
  static const char *server_certificate = "-----BEGIN CERTIFICATE-----\n"
    "MIIFHDCCBASgAwIBAgISA8cagukxHzEHVEt1xMuMm0J1MA0GCSqGSIb3DQEBCwUA\n"
    "MDMxCzAJBgNVBAYTAlVTMRYwFAYDVQQKEw1MZXQncyBFbmNyeXB0MQwwCgYDVQQD\n"
    "EwNSMTAwHhcNMjQxMTIxMTUxMjU1WhcNMjUwMjE5MTUxMjU0WjAUMRIwEAYDVQQD\n"
    "EwltaXRyYS5iaXowggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCgk0Ch\n"
    "4DPbDclT707OLX3nfUoE1F59LfPsjJ6x3MhVyGOleGSdSfGtorcAJh2HnMRD2gem\n"
    "m1kd1XQX2/dyknugTUkah9RffPtqbFv2b532quFIxzM6AM4a2akHxxnkfWLhdQ2g\n"
    "lYqCi42ojFKjZ99v+uu9Vu6Xj64FH55xNs7Q5xVX0qvQQpdcfbdfoDP4RJ6pcX8c\n"
    "qBTqNDo4wt7CY02pXcFLxVbZ4i9+N/iqvt7fM6gop8FTZGWR2t3GcfS7L7uDoH5C\n"
    "iU/lZd87S87jjVsem95O/nNmhE0U7WqnvA6JKjtMngDrPet9E/SgALzbIlT9EJ0+\n"
    "a9GZn4Lz/y5R3VcDAgMBAAGjggJHMIICQzAOBgNVHQ8BAf8EBAMCBaAwHQYDVR0l\n"
    "BBYwFAYIKwYBBQUHAwEGCCsGAQUFBwMCMAwGA1UdEwEB/wQCMAAwHQYDVR0OBBYE\n"
    "FHNE0rBUWzs9PvJ/Ml3+6zSB1zn+MB8GA1UdIwQYMBaAFLu8w0el5LypxsOkcgwQ\n"
    "jaI14cjoMFcGCCsGAQUFBwEBBEswSTAiBggrBgEFBQcwAYYWaHR0cDovL3IxMC5v\n"
    "LmxlbmNyLm9yZzAjBggrBgEFBQcwAoYXaHR0cDovL3IxMC5pLmxlbmNyLm9yZy8w\n"
    "TgYDVR0RBEcwRYIJKi5lcGEuZWR1ggsqLm1pdHJhLmJpeoIXKi5uYXR1cmFsaW5u\n"
    "b3ZhdGlvbi5vcmeCB2VwYS5lZHWCCW1pdHJhLmJpejATBgNVHSAEDDAKMAgGBmeB\n"
    "DAECATCCAQQGCisGAQQB1nkCBAIEgfUEgfIA8AB2AM8RVu7VLnyv84db2Wkum+ka\n"
    "cWdKsBfsrAHSW3fOzDsIAAABk09+HV4AAAQDAEcwRQIgayArqpxhykq4F/6HhAd1\n"
    "lJQiYNegtM1xdiTi13OmlasCIQDQgyRAwmMKeMIXRcZqNqTu8WsgQPWyTQ/qOxIh\n"
    "bebKXAB2AOCSs/wMHcjnaDYf3mG5lk0KUngZinLWcsSwTaVtb1QEAAABk09+HTgA\n"
    "AAQDAEcwRQIhAN879ukkf6FKvuCtkfa3eNeqsg8e3K08P+smoPr7NiC6AiAX4rSM\n"
    "9EVSW2A2DIG3D50vfz1qdiYBcrhKUoBl4PRRyjANBgkqhkiG9w0BAQsFAAOCAQEA\n"
    "LCCl0UMkccYHd8pui39aXU3+ud28kVEfhD/nDHaOUrH8rw8M+CO1SILTRXB0qp3L\n"
    "G42fusAXpPmwQPwcprULNr9nmmnL+s7UAXfhMCp3jcfTDunMsKVdP7/YABgGPKRb\n"
    "vsKzCBlTWA/WSBvaAGBXA+p0/EQSL6tIkU0CvfERb9XeTYqKj56KLinrrzxjfciG\n"
    "p/7ucCS/LtD475P2A+BVyrAbabhVAc9lXue6ZWOa70XaXj5J73skxB2/hezGbk7G\n"
    "PqE1zq4PVi/TwNaKRbcLDFcAZZddIxxqOe0FO+P02U/2g4St0xcchmfsNSg2izM4\n"
    "qs8ZO9aIZ7ceCyCRORd8zA==\n"
    "-----END CERTIFICATE-----";

  static HttpsOTAStatus_t otastatus;

  #ifdef SYSTEM_OTA_DEBUG
  void HttpEvent(HttpEvent_t *event) {
    switch (event->event_id) {
      case HTTP_EVENT_ERROR:        Serial.println("Http Event Error"); break;
      case HTTP_EVENT_ON_CONNECTED: Serial.println("Http Event On Connected"); break;
      case HTTP_EVENT_HEADER_SENT:  Serial.println("Http Event Header Sent"); break;
      case HTTP_EVENT_ON_HEADER:    Serial.printf("Http Event On Header, key=%s, value=%s\n", event->header_key, event->header_value); break;
      case HTTP_EVENT_ON_DATA:      Serial.print("."); break;
      case HTTP_EVENT_ON_FINISH:    Serial.println("Http Event On Finish"); break;
      case HTTP_EVENT_DISCONNECTED: Serial.println("Http Event Disconnected"); break;
      case HTTP_EVENT_REDIRECT:     Serial.println("Http Event Redirect"); break;
    }
  }
  #endif // SYSTEM_OTA_DEBUG
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

void otaSuccess() {
  #ifdef SYSTEM_OTA_DEBUG
    Serial.println("HTTP_UPDATE_OK. Rebooting to install new flash");
  #endif
  delay(1000); // Wait a second and restart (not sure if there is a reason for this delay)
  ESP.restart();
}
#ifdef SYSTEM_OTA_DEBUG
  void otaNoUpdate() {
      Serial.print("OTA: No need to update");
  }
  void otaFail() {
        Serial.println("Firmware Upgrade Fail");
  }
#endif
void updateAndReboot(bool sketch) {
  #if !defined(ESP8266) && !defined(ESP32)
    #error OTA updateAndReboot only defined so far for ESP8266 and ESP32 
  #endif

  // TODO-37 when have ESP32 OTA working gradually merge the ESP32 and ESP8266 versions.

  // Setup events etc
  #ifdef ESP32
    #ifdef SYSTEM_OTA_DEBUG
      HttpsOTA.onHttpEvent(HttpEvent);
    #endif
  #endif 

  #ifdef ESP8266
    ESPhttpUpdate.rebootOnUpdate(false); // Dont automatically reboot - do it under control below
    #ifdef SYSTEM_OTA_DEBUG
      ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW); // Note that this is subject to usual issues with LED_BUILTIN being wrong on some boards (see actuator_ledbuiltin)
      ESPhttpUpdate.onStart(update_started);
      ESPhttpUpdate.onEnd(update_finished);
      ESPhttpUpdate.onProgress(update_progress);
      ESPhttpUpdate.onError(update_error);
    #endif
  #endif

  if (sketch) { // Note !sketch would be for spiffs which isnt implemented yet
    // Note there is no correlation between the path here, and where its stored on the server which also pays attention to dev/project/node
    // Prefered way works on ESP8266 - not sure why not on ESP32 which doesn't liek F() on first arg to a concatenation
    // TODO-37 rewrite as char concatenation - and its almost certainly "const"
    #ifdef ESP32 
        const size_t buffer_size = strlen(SYSTEM_OTA_SERVERPORTPATH) + xDiscovery::topicPrefix->length() + strlen(SYSTEM_OTA_KEY) ;
        char* url = new char[buffer_size];
        strcpy(url, SYSTEM_OTA_SERVERPORTPATH);
        strcat(url, xDiscovery::topicPrefix->c_str());
        strcat(url, SYSTEM_OTA_KEY);
    #else 
      String *url = new String(F(SYSTEM_OTA_SERVERPORTPATH) + *xDiscovery::topicPrefix + SYSTEM_OTA_KEY);
    #endif
    #ifdef SYSTEM_OTA_DEBUG
      Serial.print("Attempt OTA from:"); Serial.println(url);
    #endif

    #ifdef ESP32
      HttpsOTA.begin(url, server_certificate);
      // TODO-37 probably put this in its own loop()
      while (true) {
        otastatus = HttpsOTA.status();
        if (otastatus == HTTPS_OTA_SUCCESS) { 
          otaSuccess();
          return; //TODO-37 poor way to exit
        } else if (otastatus == HTTPS_OTA_FAIL) {
          #ifdef SYSTEM_OTA_DEBUG
            otaFail();
          #endif
          return; //TODO-37 poor way to exit 
        }
        delay(1000);
      }
    #endif //ESP32
    #ifdef ESP8266
      t_httpUpdate_return ret = ESPhttpUpdate.update(net, *url, SYSTEM_OTA_VERSION);
      if (ret == HTTP_UPDATE_OK) {
        otaSuccess();
      #ifdef SYSTEM_OTA_DEBUG
        } else if (ret == HTTP_UPDATE_NO_UPDATES) { 
          otaNoUpdate();
        } else {
          otaFail();
      #endif // SYSTEM_OTA_DEBUG
        }
    #endif //ESP8266
  } else {
    Serial.println("TODO-37 not defined for SPIFFS yet");
  }
}

void setup() {
  #ifdef ESP32 // TODO-37 move this to system_time
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_netif_sntp_init(&config);
    if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000)) != ESP_OK) {
      Serial.println("Failed to update system time within 10s timeout");
    }
    time_t now;
    char strftime_buf[64];
    struct tm timeinfo;

    time(&now);
    // Set timezone to China Standard Time
    setenv("TZ", "CST-8", 1); //TODO use Indo time
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    Serial.printf("The current date/time in Shanghai is: %s", strftime_buf);
  #endif
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
