/* Frugal-IoT - WiFi handling
 *
 *
 */
#include <Arduino.h>
#include "_settings.h" // For ???
#include "system_wifi.h"
#include "misc.h" // For Sprintf
#include "system_frugal.h" // for frugal_iot
#ifdef ESP32
  #include <esp_wifi.h> // For esp_wifi_stop / start and WiFi client
#endif
#if defined(ESP8266)
  #include <ESP8266WiFi.h>  // for WiFiClient
#else
  #include <WiFi.h> // This will be platform dependent, will work on ESP32 but most likely want configurration for other chips/boards
#endif

#ifndef SYSTEM_WIFI_SCANPERIOD
  #define SYSTEM_WIFI_SCANPERIOD 20000
#endif

// Note the equivalent of System_WiFi::captiveLines is part of system_captive.cpp

System_WiFi::System_WiFi()
: System_Base("wifi", "WiFi")
  {}

#ifdef ESP8266
// F() uses a GCC statement-expression (PSTR) which is not allowed at global scope on ESP8266.
// Declare strings in PROGMEM manually and cast to __FlashStringHelper*.
static const char _wsn0[] PROGMEM = "Starting";
static const char _wsn1[] PROGMEM = "Disconnected";
static const char _wsn2[] PROGMEM = "Reconnecting";
static const char _wsn3[] PROGMEM = "Connected";
static const char _wsn4[] PROGMEM = "Needscan";
static const char _wsn5[] PROGMEM = "Scanning";
static const char _wsn6[] PROGMEM = "Scanned";
static const char _wsn7[] PROGMEM = "Connecting";
static const char _wsn8[] PROGMEM = "Stabilizing";
const __FlashStringHelper* wifiStatusNames[] = {
  FPSTR(_wsn0), FPSTR(_wsn1), FPSTR(_wsn2), FPSTR(_wsn3), FPSTR(_wsn4),
  FPSTR(_wsn5), FPSTR(_wsn6), FPSTR(_wsn7), FPSTR(_wsn8)
};
#else
const __FlashStringHelper* wifiStatusNames[] = {
  F("Starting"),
  F("Disconnected"),
  F("Reconnecting"),
  F("Connected"),
  F("Needscan"),
  F("Scanning"),
  F("Scanned"),
  F("Connecting"),
  F("Stabilizing")
};
#endif

void System_WiFi::setStatus(WiFiStatusType newstatus) {
  if (status != newstatus) {
    #ifdef SYSTEM_WIFI_DEBUG
      Serial.print(F("Wifi.status=")); Serial.print(WiFi.status());
      Serial.print(F(" WIFI ")); Serial.println(wifiStatusNames[newstatus]);
    #endif
    status = newstatus;
    statusSince = millis();
  }
}

void System_WiFi::setup() {
  readConfigFromFS(); // Note takes a slightly different format, as each file is a SSID with content = password (calls dispatchTwig with each ssid/password pair)
}
bool System_WiFi::rescan() {
  // ESP8266 scanNetworks(bool async = false, bool show_hidden = false, uint8 channel = 0, uint8* ssid = NULL);
  //bool async = false, bool show_hidden = false, bool passive = false, uint32_t max_ms_per_chan = 300, uint8_t channel = 0, const char *ssid = nullptr, const uint8_t *bssid = nullptr
  #ifdef ESP32
    esp_wifi_scan_stop(); // Abort any in-progress scan before starting a new one
    esp_wifi_disconnect(); // Ensure STA is idle — scan fails if a connect attempt is still in flight
  #endif
  WiFi.scanDelete(); // Free previous scan results and reset arduino-esp32 _scanStarted flag
  int16_t result = WiFi.scanNetworks(true); // Scan asynchronously; WIFI_SCAN_RUNNING on success
  if (result != WIFI_SCAN_RUNNING) {
    Serial.print(F("WiFi scan start failed code=")); Serial.print(result);
    Serial.print(F(" WiFi.status=")); Serial.println(WiFi.status());
    return false;
  }
  return true;
}

// Scan for networks - try and connect to any we have password for (in order of strength) true if success
// This code is hairey ! As the for-loops are run over multiple calls to this function, 
// notice how they don't set their control variable back to the start. 
bool System_WiFi::connectOneAndAllNext() {
  // Try to connect to any networks we know - in order of strength
  // Running thru strongest networks first
  for (; minRSSI > -1000; minRSSI -= 5) { // Look at ranges of RSSI in 5 unit chunks
    for (; (nextNetwork < num_networks); nextNetwork++) {  // Loop over networks we see
      if ((WiFi.RSSI(nextNetwork) > minRSSI) && (WiFi.RSSI(nextNetwork) <= (minRSSI + 5))) { // Pick any in the RSSI range
        if (connectOne(WiFi.SSID(nextNetwork), WiFi.RSSI(nextNetwork))) {
          nextNetwork++; // If fails, try next network, not same again
          return true;
        }
        // Otherwise - no known password so try next
      }
    }
    nextNetwork = 0; // tried all (if any) networks at this RSSI 
  }
  // If get to end of scan with none found return false.
  return false;
}
void System_WiFi::connectOneAndAllReset() {
  // num_networks = WiFi.scanComplete(); // Already set in state machine
  minRSSI = 0;
  nextNetwork = 0; 
  // Serial.print(F("WiFi Scan found ")); Serial.println(num_networks); // Reported in state machine
}
bool System_WiFi::connectOne(String ssid, int32_t rssi) {
  Serial.print(ssid); Serial.print(F(" ")); if (rssi) { Serial.print(rssi); }; Serial.print(F(" "));
  String filename = String("/wifi/") + ssid ;
  String pw = frugal_iot.fs_LittleFS->slurp(filename, true);
  if (pw.length()) { // Do we have a password
    connectInnerAsync(ssid, pw); // Try and connect
    return true; // Drop out - state retained for next call which will happen after it succeeds or fails to connect
  } else {
    Serial.println(F("Unknown"));
    return false;
  }
}
// Try and connect to a single network
void System_WiFi::connectInnerAsync(String ssid, String pw) {
  Serial.print(F("Connecting to WiFi SSID "));
  Serial.println(ssid);
  // mode() and config() must be called BEFORE begin() — IDF 5.x rejects them once connecting has started
  #ifdef ESP32
    WiFi.mode(WIFI_MODE_APSTA);  // Keep captive portal alive; arduino-esp32 #6278
  #else
    WiFi.mode(WIFI_AP_STA);
  #endif
  WiFi.setHostname(frugal_iot.nodeid.c_str());
  #ifndef ESP8266
    // This won't work on ESP8266 which is unforgiving - if dont set all four then Serial.print(WiFi.localIP()) will report IP unset and DNS lookup will fail
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);  // arduino-esp32 #2537 and #6278
  #endif
  WiFi.setHostname(frugal_iot.nodeid.c_str());
  if ((wl_status_t)WiFi.begin(ssid.c_str(), pw.c_str()) == WL_CONNECT_FAILED) {
    // WiFi.begin() failed synchronously — IDF rejected esp_wifi_set_config() (ESP_ERR_WIFI_STATE).
    // WiFi stack is still processing a prior disconnect; set flag so WIFI_CONNECTING exits fast.
    Serial.println(F("WiFi.begin state err - still stabilizing"));
    _connectFailed = true;
    stabilizeTill = millis() + 5000;
  }
}


#ifdef UNUSED_WILL_USE_WITH_POST_BUT_NEEDS_ASYNC
// TODO-153 - call something like this after POST only
// Attempt to connect to main network as configured, if succeed save the id and password as a known network
bool System_WiFi::connect1(String ssid, String pw, int wait_seconds) {
  if (connectInner(ssid, pw)) {
    addWiFi(ssid, pw); // Save for later
    return true;
  }
  return false;
}
#endif

void System_WiFi::addWiFi(String ssid, String password) {
  writeConfigToFS(ssid, password);  // /wifi/<ssid> = password
}

#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32
// see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/power_management.html for how to keep WiFi alive during sleep
bool System_WiFi::pauseWiFi() {
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html
   return (esp_wifi_stop() == ESP_OK); // Suggested to reduce dropping WiFi connection
}
#endif

#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32
bool System_WiFi::recoverWiFi() {
  if (esp_wifi_start() != ESP_OK) {
    Serial.println(F("Failed to restart esp_wifi"));
    return false;
  }
   // Spin till its started
  while (WiFi.status() == WL_NO_SHIELD) {
     /*Serial.print(F("w"));*/ 
     delay(100); 
  }
  return true;
  // Recover WiFi in stateMachine 
}
#endif // ESP32

#ifdef WL_STOPPED
  #define WL_EXPECTED_AT_STARTING WL_STOPPED
#else
  #define WL_EXPECTED_AT_STARTING WL_DISCONNECTED // e.g. on ESP8266
#endif
void System_WiFi::stateMachine() {
  // State machine — stabilizeTill blocks all transitions while waiting for WiFi stack to settle
  //Serial.print(F(" XXX Wifi=")); Serial.print(status); Serial.print(F(" ")); Serial.println(WiFi.status());
  if (millis() >= stabilizeTill) {
    switch (status)
    {
      case WIFI_STARTING: //0
        if ((WiFi.status() != WL_DISCONNECTED) // Note that WL_DISCONNECTED is 7 on ESP8266 and 6 on ESP32
          #ifdef ESP32
            && (WiFi.status() != WL_STOPPED) // WL_STOPPED is not defined on ESP8266 (its 254 on ESP32)
          #endif
          ) {
          #ifdef SYSTEM_WIFI_DEBUG
            Serial.print(F("WiFi: STARTING but WiFi.status=")); Serial.println(WiFi.status());
            // Unsure what to do here -
          #endif
        }
        setStatus(WIFI_NEEDSCAN);
        break;
      case WIFI_DISCONNECTED: //1
        if ( (WiFi.status() != WL_DISCONNECTED)
          && (WiFi.status() != WL_CONNECTION_LOST)
          && (WiFi.status() != WL_IDLE_STATUS)
          && (WiFi.status() != WL_CONNECT_FAILED)
        ) {
          #ifdef SYSTEM_WIFI_DEBUG
            Serial.print(F("XXX Should be WL_DISCONNECTED or WL_STOPPED but")); Serial.println(WiFi.status());
            // Unsure what to do here - but try and reconnect anyway
          #endif
        }
        setStatus(WIFI_RECONNECTING);
        WiFi.reconnect();  // In theory should be quick if not possible.
          // drop thru
      case WIFI_RECONNECTING: //2
          if (WiFi.status() == WL_CONNECTED) {
            setStatus(WIFI_CONNECTED);
            break; // Success
          }
          if (millis() < (statusSince + 3000)) { // Failed to reconnect
            break; // Leave at WIFI_RECONNECTING
          }
          Serial.println(F("WiFi failed to reconnect"));
          setStatus(WIFI_NEEDSCAN);
          // drop thru
      case WIFI_NEEDSCAN: //4
          if (rescan()) { // async
            setStatus(WIFI_SCANNING);
            Serial.print(F("WiFi rescanning ")); /* Serial.println(WiFi.status()); */
          } else {
            stabilizeTill = millis() + 2000; // Retry in 2s, not on every loop tick
            break; // stay in WIFI_NEEDSCAN
          }
          // drop thru
      case WIFI_SCANNING: //5
        switch (num_networks = WiFi.scanComplete()) {
          case WIFI_SCAN_FAILED: {
            Serial.print(F("WiFi Scan failed to complete"));
            status = WIFI_NEEDSCAN;
            break;
          }
          case WIFI_SCAN_RUNNING: {
            break; // No need for timeout, it will terminate
          }
          default: {
            // Weirdly WiFi.status() reports WL_DISCONNECTED rather than WL_SCAN_COMPLETED
            Serial.print(F("WiFi found:")); Serial.println(num_networks);
            connectOneAndAllReset();
            status = WIFI_SCANNED;
          }
        }
        break;
        // Drop thru
      case WIFI_SCANNED: //6 Each time it hits this, it will try and connect to one more node if possible
        if (!connectOneAndAllNext()) { // returns either true if started connecting, or false if nothing to try
          // Came to end but none found
          if (millis() > (statusSince + SYSTEM_WIFI_SCANPERIOD)) { // Leave space between scans or portal wont work
            setStatus(WIFI_NEEDSCAN);
          }
          break; // Either still in WIFI_SCANNED at end of tries, OR with WIFI_NEEDSCAN
        } else {
          setStatus(WIFI_CONNECTING);
        }
        // Drop thru
      case WIFI_CONNECTING: //7
        if (WiFi.status() == WL_CONNECTED) {
          setStatus(WIFI_STABILIZING);
          stabilizeTill = millis() + 2000; // Give WiFi stack time to settle before setup_after_wifi
        } else if (_connectFailed) {
          // connectInnerAsync() detected WiFi.begin() returned WL_CONNECT_FAILED (IDF state error).
          // WiFi is still connecting from a prior attempt; abort it and wait before retrying.
          _connectFailed = false;
          #ifdef ESP32
            {
              esp_err_t err = esp_wifi_disconnect();
              #ifdef SYSTEM_WIFI_DEBUG
                if (err != ESP_OK) { Serial.print(F("esp_wifi_disconnect err ")); Serial.println(err); }
              #endif
            }
          #endif
          stabilizeTill = millis() + 5000;
          setStatus(WIFI_SCANNED);
        } else if (millis() > (statusSince + 30000)) { // Give it 30 seconds to try
          #ifdef ESP32
            // WiFi.disconnect(true,true) fails on IDF 5.x while connecting (ESP_ERR_WIFI_STATE).
            // esp_wifi_disconnect() aborts in-progress attempt and works in any state.
            {
              esp_err_t err = esp_wifi_disconnect();
              #ifdef SYSTEM_WIFI_DEBUG
                if (err != ESP_OK) { Serial.print(F("esp_wifi_disconnect err ")); Serial.println(err); }
              #endif
            }
          #endif
          stabilizeTill = millis() + 5000; // Give WiFi task time to process disconnect before next begin()
          setStatus(WIFI_SCANNED); // Try next network from current scan results, not a full rescan
        }
        // WiFi.status() remains at 6 during this timeout - cant tell quicker that it failed
        break; // Either as WIFI_CONNECTED | WIFI_CONNECTING | WIFI_SCANNED
      case WIFI_STABILIZING: //8
        if (WiFi.status() != WL_CONNECTED) {
          setStatus(WIFI_DISCONNECTED);
          break;
        }
        // stabilizeTill guard above already waited 2000ms — proceed directly
        setStatus(WIFI_CONNECTED);
        frugal_iot.setup_after_wifi(); // Callback to MQTT, Time and OTA as first connected
        //drop thru
      case WIFI_CONNECTED: //3
        if (WiFi.status() != WL_CONNECTED) {
          setStatus(WIFI_DISCONNECTED);
          Serial.print(F("WiFi connection lost, WiFi.status=")); Serial.println(WiFi.status());
        }
        break;
      default:
          Serial.println(F("WiFi: unhandled status"));
        break;
    }
  }
  //WiFi.softAPgetStationNum() maybe relevant - but prob not - its number connected to captive portal
}
// This should not "yield" or "delay" as called from inside Captive, if that is a problem queue it as a message
void System_WiFi::switchSSID(const String ssid) {
  if (ssid != WiFi.SSID()) {
    WiFi.disconnect(); // Disconnect if connected.
    if (connectOne(ssid)) { // Will be false if no password
      setStatus(WIFI_CONNECTING); 
      connectOneAndAllReset(); // If connect fails, start search at top of scan
    }
  }
}

int8_t System_WiFi::RSSI() { // Negative number if decibel-milliwatts
  return WiFi.RSSI(); 
}

uint8_t System_WiFi::rssi_to_bars(int8_t rssi) {
  // There is no standard but various searches turned up this range
  if (rssi > -43) return 5;
  if (rssi > -60) return 4;
  if (rssi > -68) return 3;
  if (rssi > -75) return 2;
  if (rssi > -88) return 1;
  return 0;
}
uint8_t System_WiFi::bars() {
  return rssi_to_bars(WiFi.RSSI());
}
String System_WiFi::SSID() {
  return WiFi.SSID();
}
bool System_WiFi::connected() {
  return (status == WIFI_CONNECTED); 
}

unsigned long lasttime = 0L;

void System_WiFi::loop() {
  if (millis() > (lasttime + 100)) { // Unclear what right time delay is here - 2000 works - so does 100
    lasttime = millis();
    stateMachine();
  }
}
void System_WiFi::dispatchTwig(const String &topicSensorId, const String &topicTwig, const String &payload, bool isSet) {
  // Setting on wifi e.g. esp1234/set/wifi/foo/bar is setting the wifi password to "bar" for ssid=foo
  // No need to echo this to the UX
  // Note this can only be set from the captive, or the SPIFFS
  if (isSet && (topicSensorId == id)) {
    // topicTwig is ssid payload is password
    if (payload.length()) {  // Only save if have a password
      addWiFi(topicTwig, payload);
    }
    // Intentionall not //System_Base::dispatchTwig(topicSensorId, topicTwig, payload, isSet);
  }
}
 