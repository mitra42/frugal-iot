/* OTA client
* 
* Please see https://github.com/mitra42/frugal-iot/blob/main/docs/ota.md and 
* and https://github.com/mitra42/frugal-iot/issues/37
*
* Configuration

* Required: SYSTEM_OTA_PREFIX short string for different apps e.g. sht30
* Required: SYSTEM_OTA_SUFFIX short string for boards e.g. d1_mini
* Optional: SYSTEM_OTA_DEBUG SYSTEM_OTA_MS SYSTEM_OTA_SERVERPORTPATH 
* 
* OTA was a pain to implement - had to move server to https, then it wouldn't connect and most of the published 
* examples didn't work - thanks to Jonathan Semple for a working solution, now incorporated here. 
*/

#include "_settings.h"
#if defined(SYSTEM_OTA_PREFIX) && defined(SYSTEM_OTA_SUFFIX)

#include <Arduino.h>
#include "system_discovery.h"
#include "system_wifi.h"
#include "system_ota.h"
#include "system_frugal.h"
#include "actuator_ledbuiltin.h" // for correct LED_BUILTIN

// Note that OTA will fail (no space) on devices where the code is >50% of space - currently this includes SONOFFs
#ifdef ESP8266
  #include <ESP8266httpUpdate.h> // defines ESPhttpUpdate
  #include <ESP8266WiFi.h>  // for WiFiClient
  #define HTTPUPDATE ESPhttpUpdate
#elif defined(ESP32)
  #include <WiFi.h> // This will be platform dependent, will work on ESP32 but most likely want configurration for other chips/boards
  #include <HTTPClient.h> // espressif / Arduino-ESP32 library
  #include <HTTPUpdate.h> // espressif / Arduino-ESP32 library
  #include <WiFiClientSecure.h>
  #define HTTPUPDATE httpUpdate
  #define SYSTEM_OTA_USECERT
#else
    #error OTA only defined so far for ESP8266 and ESP32 
#endif

#include "system_frugal.h" // For sleepSafemillis()

#ifndef SYSTEM_OTA_VERSION
  #define SYSTEM_OTA_VERSION "0.0.0" // We dont use versions, we are using MD5
#endif

#ifndef SYSTEM_OTA_SERVERPORTPATH
  #define SYSTEM_OTA_SERVERPORTPATH "https://frugaliot.naturalinnovation.org/ota_update/" // Note trailing slash
#endif

#ifdef SYSTEM_OTA_USECERT

  const char* rootCACertificateForNaturalInnovation = \
      "-----BEGIN CERTIFICATE-----\n"
  "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\n"
  "-----END CERTIFICATE-----\n";
#endif

System_OTA::System_OTA() 
: System_Base("ota", "OTA"), 
  timer_index(frugal_iot.powercontroller->timer_next()) 
{ }

System_OTA::~System_OTA() { }

void System_OTA::init(const String otaServerAddress, const String softwareVersion, const char* caCert) {
  _otaServerAddress = otaServerAddress;
  _softwareVersion = softwareVersion;
  _isOK = true;
  _retryCount = 3;
  #ifdef SYSTEM_OTA_USECERT
    _caCert = caCert;
  #endif
}

void otaStartCB() {
  frugal_iot.ota->_isOK = true;
  Serial.println(F("OTA start"));
}

void otaProgressCB(int done, int size) {
  Serial.print(F("OTA progress: Done ")); Serial.print(done); Serial.print(F(" of ")); Serial.println(size);
}

void otaEndCB() {
  Serial.println(F("OTA end"));
  frugal_iot.ota->_checked = true;
}

void otaErrorCB(int errorCode) {
  Serial.print(F("OTA error ")); Serial.println(errorCode);
  frugal_iot.ota->_isOK = false;
}

void System_OTA::checkForUpdate() {
  #ifdef ESP32
    WiFiClientSecure client;
  #elif defined(ESP8266)
    WiFiClientSecure client; // Assumes system_wifi has already connected to access point - note this will not run if WiFi fails to connect and goes to portal mode
  #endif
    //TODO On ESP8266 If we set the certificate then it throws an exception ... dont really need it. 
  #ifdef SYSTEM_OTA_USECERT
    #ifdef ESP32
      client.setCACert(_caCert);
    #elif defined(ESP8266)
      // TODO This crashed when tested but might actually be OK 
      client.setTrustAnchors(new X509List(_caCert));
    #endif
  #else
    client.setInsecure(); // Alternative
  #endif
  client.setTimeout(20000);
  // Set this if you want your OTA server to be able to return a redirect to force devices to collect the firmware from e.g. githubraw
  HTTPUPDATE.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS); 

  HTTPUPDATE.setLedPin(LED_BUILTIN, LOW); // Note that this is subject to usual issues with LED_BUILTIN being wrong on some boards (see actuator_ledbuiltin)
  HTTPUPDATE.onStart(otaStartCB);
  HTTPUPDATE.onProgress(otaProgressCB);
  HTTPUPDATE.onEnd(otaEndCB);
  HTTPUPDATE.onError(otaErrorCB);

  // It is possible to send an additional "currentVersion" string that is sent as a header "x-ESP32-version"
  // but that doesn't seem to have any advantages over just using the MD5 that is sent anyway.
  // As it's there I might as well send the user-friendly sw version string such as 1.0.1
  // but the true key to the software is the MD5 hash, which is always sent
  
  if (_otaServerAddress.length() == 0 || ! strchr(_otaServerAddress.c_str(), ':')) {
    Serial.println(F("OTA ServerUrl not set"));
    return;
  }
  t_httpUpdate_return ret = HTTPUPDATE.update(client, _otaServerAddress, _softwareVersion);

  switch (ret){
    case HTTP_UPDATE_FAILED:
  	  Serial.print(F("OTA error =")); Serial.print(HTTPUPDATE.getLastError()); Serial.print(F(",")); Serial.println(HTTPUPDATE.getLastErrorString());
      _isOK = false;
      break;
    
    case HTTP_UPDATE_NO_UPDATES:
	    Serial.println(F("OTA up to date"));
      _checked = true;
      _isOK = true;
      break;
    
    case HTTP_UPDATE_OK:
	    Serial.println(F("OTA end"));
      _checked = true;
      _isOK = true;
      break;
  }
  _retryCount -= 1;
}

const String System_OTA::getOTApath() {
    // Note there is no correlation between the path here, and where its stored on the server which also pays attention to dev/project/node
    return String(SYSTEM_OTA_SERVERPORTPATH + frugal_iot.messages->topicPrefix + SYSTEM_OTA_PREFIX "_" SYSTEM_OTA_SUFFIX);
}

void System_OTA::setup_after_mqtt_setup() {
  // Nothing to read from disk so not calling readConfigFromFS 
  const String url = getOTApath(); // Needs topicPrefix setup in MQTT::setup
  // Note this must run after WiFi has connected  and ideally before MQTT or Discovery except it needs xDiscovery::topicPrefix
  Serial.print(F("Attempt OTA from:")); Serial.println(url);
  #ifdef SYSTEM_OTA_USECERT
    init(url, SYSTEM_OTA_VERSION, rootCACertificateForNaturalInnovation);
  #else
    init(url, SYSTEM_OTA_VERSION, nullptr);
  #endif

  // Note - not doing here as WiFi won't be setup yet
  //checkForUpdate();
}

//TODO-194 needs testing
void System_OTA::infrequently() {
  if (frugal_iot.canOTA() && frugal_iot.powercontroller->timer_expired(timer_index)) {
    checkForUpdate();
    frugal_iot.powercontroller->timer_set(timer_index, SYSTEM_OTA_MS);
  }
}

void System_OTA::discover() {
  frugal_iot.messages->send(leaf2path("key"), String(SYSTEM_OTA_PREFIX "_" SYSTEM_OTA_SUFFIX ), MQTT_RETAIN, MQTT_QOS_ATLEAST1);
}

#endif // SYSTEM_OTA_PREFIX && SYSTEM_OTA_SUFFIX
