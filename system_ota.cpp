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

const char* rootCACertificateForNaturalInnovation = \
    "-----BEGIN CERTIFICATE-----\n"
"MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw\n"
"TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n"
"cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4\n"
"WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu\n"
"ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY\n"
"MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc\n"
"h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+\n"
"0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U\n"
"A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW\n"
"T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH\n"
"B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC\n"
"B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv\n"
"KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn\n"
"OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn\n"
"jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw\n"
"qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI\n"
"rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV\n"
"HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq\n"
"hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL\n"
"ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ\n"
"3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK\n"
"NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5\n"
"ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur\n"
"TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC\n"
"jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc\n"
"oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq\n"
"4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA\n"
"mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d\n"
"emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=\n"
"-----END CERTIFICATE-----\n";

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
      HttpsOTA.begin(url, rootCACertificateForNaturalInnovation);
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
  /* DOesnt compile - replace with John's code but put in system_time
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
  */
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
