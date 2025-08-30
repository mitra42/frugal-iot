/* Frugal IoT - Google Sheets
 * 
 * Idea is to be able to log any MQTT topic - local or remote 
 * to a Google Sheet
 * 
 * See https://github.com/mitra42/frugal-iot/issues/136
 * 
 * Thanks to Abrar at Kopernik for the idea
*/
#include "_settings.h"

#ifdef ESP8266
  #include <ESP8266HTTPClient.h> // For ESP8266
  #include <ESP8266WiFi.h>  // for WiFiClient
#else // ESP32  
  #include <HTTPClient.h>
#endif
#include <Arduino.h> // For String
#include "control_gsheets.h"
#include "misc.h" // For StringF
#include "system_frugal.h"

Control_Gsheets::Control_Gsheets(const char* name, const String googleSheetsUrl)
  : Control_Logger("gsheets", name), url(googleSheetsUrl)
  {}
  
Control_Gsheets::Control_Gsheets(const char* name, const char* const googleSheetsUrl)
  : Control_Gsheets(name,  String(googleSheetsUrl))
  {}

void Control_Gsheets::track(const char* col, String topicPath) {
  // TODO-136 may not want to pass empty new string here
  INtext* i = new INtext(id, col, col, String(), "black", true); // sensorId, id, name, value, color, wireable 
  i->wireTo(topicPath); // Does a subscription
  inputs.push_back(i);
}

void Control_Gsheets::track(const char* col, const char* topicPath) {
  track(col, String(topicPath));
}

void Control_Gsheets::act() {
  String payload = String(F("{\"timestamp\":\""))
  + frugal_iot.time->dateTime()
  + "\"";
  // For some reason this alternative doesnt work - would have expected dateTime to be in scope but StringF is fussy.
  //String dateTime = systemTime.dateTime();
  //*payload += StringF("{\"timestamp\":\"%s\"", dateTime); // TODO-136 check this time is recognized by gsheets
  for (auto &input : inputs) {
    // TODO-136 may be a problem quoting output if it, for example, is a float
    payload += StringF(",\"%s\":\"%s\"", 
      input->id, 
      input->StringValue().c_str());
  }
  payload += "}";
  sendGoogle(payload);
}

void Control_Gsheets::sendGoogle(String payload) {
    HTTPClient http;
  #ifdef ESP8266
    WiFiClient client; // Assumes system_wifi has already connected to access point - note this will not run if WiFi fails to connect and goes to portal mode
    http.begin(client, url);
  #else
    http.begin(url);
  #endif
    http.addHeader("Content-Type", "application/json");
    Serial.print(F("📤 Sending Data to Google Sheet at:")); Serial.println(url);
    Serial.println(payload);  // Debug: Print JSON payload
    int httpResponseCode = http.POST(payload);  
    if (httpResponseCode >= 300 && httpResponseCode < 400) {
        Serial.println(F("✅ Data Sent Successfully to Google Sheets!"));
        Serial.println(httpResponseCode);
    } else {
        Serial.printf("❌ Error Sending Data! HTTP Code: %d\n", httpResponseCode);
    }
    http.end();
}

