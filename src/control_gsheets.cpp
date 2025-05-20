/* Frugal IoT - Google Sheets
 * 
 * Idea is to be able to log any MQTT topic - local or remote 
 * to a Google Sheet
 * 
 * Thanks to Abrar at Kopernik for the idea
*/
#include "_settings.h"

#ifdef CONTROL_GSHEETS_WANT

#include "system_time.h"
#include <HTTPClient.h>
#include "control.h"
#include "control_gsheets.h"

Control_Gsheets::Control_Gsheets(const char* name, String* googleSheetsUrl)
  : Control("gsheets", name,  std::vector<IN*> {},   std::vector<OUT*> {}), googleSheetsUrl(googleSheetsUrl)
  {}


void Control_Gsheets::track(const char* row, String* topicPath) {
  INtext i = INtext(id, row, row, nullptr, nullptr, true); // sensorId, id, name, value, color, wireable 
  i->wireTo(topicPath); // Does a subscription
  inputs.push_back(i);
}
void Control_Gsheets::track(IOtype t, const char* row, const char* topicPath) {
  track(new String(topicPath));
}

void Control_Gsheets::act() {
  String* payload = StringF("{\"timestamp\":\"%s\"", 
    systemTime.dateTime()); // TODO-136 check this time is recognized by gsheets
  for (auto &input : inputs) {
    // TODO-136 may be a problem quoting output if it, for example, is a float
    payload += StringF(",\"%s\":\"%s\"", 
      input->row, 
      input->textValue());
  }
  payload += "}";
  sendGoogle(payload);
};
void Control_Gsheets::sendGoogle(String* payload) {
    HTTPClient http;
    http.begin(googleSheetsUrl);
    http.addHeader("Content-Type", "application/json");
    Serial.print("📤 Sending Data to Google Sheet at:"); Serial.println(sheetUrl);
    Serial.println(payload);  // Debug: Print JSON payload
    int httpResponseCode = http.POST(payload);  
    if (httpResponseCode > 0) {
        Serial.println("✅ Data Sent Successfully to Google Sheets!");
    } else {
        Serial.printf("❌ Error Sending Data! HTTP Code: %d\n", httpResponseCode);
    }
    http.end();
}


#endif // CONTROL_GSHEETS_WANT
