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

System_WiFi::System_WiFi()
: System_Base("wifi", "WiFi")
  {}

void System_WiFi::setStatus(WiFiStatusType newstatus) {
  if (status != newstatus) {
    status = newstatus;
    statusSince = millis();
  }
}

void System_WiFi::setup() {
  readConfigFromFS();
}
bool System_WiFi::rescan() {
  // ESP8266 scanNetworks(bool async = false, bool show_hidden = false, uint8 channel = 0, uint8* ssid = NULL);
  //bool async = false, bool show_hidden = false, bool passive = false, uint32_t max_ms_per_chan = 300, uint8_t channel = 0, const char *ssid = nullptr, const uint8_t *bssid = nullptr
  // TODO-153 check if need this to scan as concerned disconnects AP
  /*
  #ifdef ESP32
    WiFi.disconnect(true, true);    // reset state so .scanNetworks() works
  #else
      WiFi.disconnect(true);
  #endif
  */
  return (WiFi.scanNetworks(true) == WIFI_SCAN_RUNNING); // Scan asynchronously. look for WL_SCAN_COMPLETED when done
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
  // Unclear why this brute multiple setHostname calls - should be documented?
  Serial.print(F("Connecting to WiFi SSID "));
  Serial.print(ssid);
  WiFi.setHostname(frugal_iot.nodeid.c_str());  
  WiFi.begin(ssid.c_str(), pw.c_str()); // Attempt connection - note returns immediately need to check status
  WiFi.setHostname(frugal_iot.nodeid.c_str());  
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);  // arduino-esp32 #2537 and #6278
  WiFi.setHostname(frugal_iot.nodeid.c_str());  
  // Dont drop captive portal while trying to connect
  #ifdef ESP32
    WiFi.mode(WIFI_MODE_APSTA);  // arduino-esp32 #6278.  WIFI_MODE_STA is just station, AP is just Access Point
  #else
    WiFi.mode(WIFI_AP_STA);  // On ESP8266 WIFI_MODE_APSTA doesnt exist its WIFI_AP_STA or WIFI_STA
  #endif
  WiFi.setHostname(frugal_iot.nodeid.c_str());  
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
bool System_WiFi::prepareForLightSleep() {
   return (esp_wifi_stop() == ESP_OK); // Suggested to reduce dropping WiFi connection
}
#endif

#ifdef ESP32 // Deep, Light and Modem sleep specific to ESP32
bool System_WiFi::recoverFromLightSleep() {
  if (esp_wifi_start() != ESP_OK) {
    Serial.println(F("Failed to restart esp_wifi"));
    return false;
  }
   // Spin till its started
  while (WiFi.status() == WL_NO_SHIELD) {
     /*Serial.print("w");*/ 
     delay(100); 
  }
  return true;
  // Recover WiFi in stateMachine // TODO-153
}
#endif // ESP32

void System_WiFi::stateMachine() {
  // State machine
  //Serial.print(" XXX Wifi="); Serial.print(status); Serial.print(" "); Serial.println(WiFi.status());
  switch (status)
  {
    case WIFI_STARTING: //0
      #ifdef ESP8266
        // WL_STOPPED doesnt exist on ESP8266 - check what it should be at startup
        Serial.print("XXX add check for WiFi.status() = "); Serial.println(WiFi.status());
      #else
        if (WiFi.status() != WL_STOPPED ) {
          #ifdef SYSTEM_WIFI_DEBUG
            Serial.print(F("XXX Should be WL_STOPPED but")); Serial.println(WiFi.status());
            // Unsure what to do here - 
          #endif
        }
      #endif
      setStatus(WIFI_NEEDSCAN);
      break;
    case WIFI_DISCONNECTED: //1
      if (WiFi.status() != WL_DISCONNECTED) {
        #ifdef SYSTEM_WIFI_DEBUG
          Serial.print(F("XXX Should be WL_DISCONNECTED or WL_STOPPED but")); Serial.println(WiFi.status());
          // Unsure what to do here - 
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
          Serial.print("WiFi rescanning "); Serial.println(WiFi.status());
        } else {
          Serial.println("WiFi Scan failed to start");
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
          Serial.print("WiFi found:"); Serial.println(num_networks);
          connectOneAndAllReset(); 
          status = WIFI_SCANNED;
        }
      }
      break;
      // Drop thru      
    case WIFI_SCANNED: //6 Each time it hits this, it will try and connect to one more node if possible
      if (!connectOneAndAllNext()) { // returns either true if started connecting, or false if nothing to try
        // Came to end but none found
        if (millis() > (statusSince + 5000)) { // Leave space between scans or portal wont work 
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
      } else if (millis() > (statusSince + 30000)) { // Give it 30 seconds to try
        // Failed to connect - if don't do this heavy disconnect it will fail to scan.
        WiFi.disconnect(true, true); // Shouldnt need to do this //TODO-153 if keep then only oor ESP32
        setStatus(WIFI_SCANNED); // Go back for next
      }
      // WiFi.status() remains at 6 during this timeout - cant tell quicker that it failed
      break; // Either as WIFI_CONNECTED | WIFI_CONNECTING | WIFI_SCANNED
    case WIFI_STABILIZING: //8
      if (WiFi.status() != WL_CONNECTED) {
        setStatus(WIFI_DISCONNECTED);
        break;
      // TODO-153 Waiting for stabilization - maybe could be shorter but noticed at 1000 that first MQTT connect often faiks
      } else if (millis() > (statusSince + 2000)) {
        setStatus(WIFI_CONNECTED);
        frugal_iot.setup_after_wifi(); // Callback to MQTT as first connected
        //drop thru
      }
      // drop thru
    case WIFI_CONNECTED: //3
      if (WiFi.status() == WL_DISCONNECTED) {
        setStatus(WIFI_DISCONNECTED);
      } else if (WiFi.status() != WL_CONNECTED) {
        Serial.print(F("WiFi: unhandled state combination CONNEcTED but WiFi.status=")); Serial.println(WiFi.status());
      }
      break;
    default:
        Serial.println(F("WiFi: unhandled status"));
      break;
  }
  //WiFi.softAPgetStationNum() maybe relevant - but prob not - its number connected to captive portal
}
void System_WiFi::switchSSID(const String ssid) {
  if (ssid != WiFi.SSID()) {
    WiFi.disconnect(); // Disconnect if connected.
    if (connectOne(ssid)) { // Will be false if no password
      setStatus(WIFI_CONNECTING); 
      connectOneAndAllReset(); // If connect fails, start search at top of scan
    }
  }
}

bool System_WiFi::connected() {
  return (status == WIFI_CONNECTED); 
}
void System_WiFi::loop() {
  static unsigned long lasttime = millis();
  if (millis() > (lasttime + 100)) { // Unclear what right time delay is here - 2000 works - so does 100 
    lasttime = millis();
    stateMachine();
  } 
}
void System_WiFi::dispatchTwig(const String &topicSensorId, const String &topicTwig, const String &payload, bool isSet) {
  if (isSet && (topicSensorId == id)) {
    // topicTwig is ssid payload is password
    if (payload.length()) {  // Only save if have a password
      addWiFi(topicTwig, payload);
    }
    // Intentionall not //System_Base::dispatchTwig(topicSensorId, topicTwig, payload, isSet);
  }
}
 