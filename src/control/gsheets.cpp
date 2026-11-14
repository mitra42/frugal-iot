/* Control_Gsheets - see gsheets.h */
#include "_settings.h"
#include <Arduino.h>
#include "control/gsheets.h"
#include "misc.h" // For StringF
#include "system/frugal.h"

Control_Gsheets::Control_Gsheets(const char* name, const String url, bool everyCycle)
  : Control_HttpPost("gsheets", name, url, everyCycle)
  {}
Control_Gsheets::Control_Gsheets(const char* name, const char* const url, bool everyCycle)
  : Control_Gsheets(name, String(url), everyCycle)
  {}

void Control_Gsheets::setup() {
  if (!frugal_iot.time) {
    Serial.println(F("Control_Gsheets: Time must be set up before Control_Gsheets"));
  }
  Control_HttpPost::setup();
}

String Control_Gsheets::body() {
  String payload = String(F("{\"timestamp\":\"")) + frugal_iot.time->dateTime() + "\"";
  for (auto &input : inputs) {
    // Every value quoted - a float arrives as a string, and the sheet converts it
    payload += StringF(",\"%s\":\"%s\"", input->id, input->StringValue().c_str());
  }
  payload += "}";
  return payload;
}
const char* Control_Gsheets::contentType() {
  return "application/json";
}
