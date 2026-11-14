/* Control_HttpPost - see httppost.h */
#include "_settings.h"
#ifdef ESP8266
  #include <ESP8266HTTPClient.h>
  #include <ESP8266WiFi.h>  // WiFiClient and WiFiClientSecure
#else
  #include <HTTPClient.h>
  #include <WiFiClientSecure.h>
#endif
#include <Arduino.h>
#include "control/httppost.h"
#include "system/frugal.h"

Control_HttpPost::Control_HttpPost(const char* id, const char* name, const String url, bool everyCycle)
  : Control(id, name, std::vector<IN*> {}, std::vector<OUT*> {}),
    url(url),
    everyCycle(everyCycle)
  {}

INtext* Control_HttpPost::addInput(const char* col) {
  INtext* i = new INtext(id, col, col, String(), "black", true); // sensorId, id, name, value, color, wireable
  inputs.push_back(i);
  return i;
}
INtext* Control_HttpPost::track(const char* col, const String topicPath) {
  INtext* i = addInput(col);
  i->wireFrom(topicPath);
  return i;
}
INtext* Control_HttpPost::track(const char* col, OUT* out) {
  INtext* i = addInput(col);
  i->wireFrom(out);
  return i;
}
INtext* Control_HttpPost::track(const char* col, System_Base* module) {
  INtext* i = addInput(col);
  i->wireFrom(module);
  return i;
}

void Control_HttpPost::act() {
  changed = true; // loop() does the rest - never post from inside a dispatch
}
void Control_HttpPost::periodically() {
  // Not restarting the clock on a post already pending: periodically() keeps running every cycle
  // while sleep is refused, and a cycle shorter than the timeout would otherwise hold the node
  // awake for as long as WiFi stayed away
  if (everyCycle && !pending) {
    pending = true;
    pendingSince = millis();
  }
}
/* A post is wanted (pending) once an input has changed, or every cycle. It is made when WiFi is
 * up, and dropped if that has not happened within CONTROL_HTTPPOST_TIMEOUT_MS - the next cycle's
 * reading is more use than a stale one.
 */
void Control_HttpPost::loop() {
  if (!everyCycle && changed && !pending) {
    pending = true;
    pendingSince = millis();
  }
  if (pending) {
    const unsigned long waited = millis() - pendingSince;
    // everyCycle: this cycle's readings arrive a loop() or two after periodically() - give them a moment
    const bool settled = changed || !everyCycle || (waited >= CONTROL_HTTPPOST_SETTLE_MS);
    if (settled) {
      bool done = false;
      if (frugal_iot.wifi->connected()) {
        post();
        done = true;
      } else if (waited >= CONTROL_HTTPPOST_TIMEOUT_MS) {
        Serial.print(id); Serial.println(F(": no WiFi - post dropped"));
        done = true;
      }
      if (done) {
        pending = false;
        changed = false;
      }
    }
  }
}
bool Control_HttpPost::okToSleep(const unsigned long ms, const uint16_t how) {
  bool ok = true;
  if (how & (PauseWiFiBit | LightSleepBit | DeepSleepBit)) {
    ok = !pending; // Bounded - loop() drops the post after CONTROL_HTTPPOST_TIMEOUT_MS
  }
  return ok;
}

String Control_HttpPost::body() {
  String b;
  for (auto &input : inputs) {
    if (b.length()) {
      b += '&';
    }
    b += input->id;
    b += '=';
    b += input->StringValue();
  }
  return b;
}
const char* Control_HttpPost::contentType() {
  return "application/x-www-form-urlencoded";
}

bool Control_HttpPost::post() {
  const String payload = body();
  WiFiClient* client;
  if (url.startsWith(F("https://"))) {
    WiFiClientSecure* secure = new WiFiClientSecure();
    secure->setInsecure(); // No certificate check - see httppost.h
    client = secure;
  } else {
    client = new WiFiClient();
  }
  HTTPClient http;
  http.setTimeout(CONTROL_HTTPPOST_HTTP_TIMEOUT_MS);
  http.begin(*client, url);
  http.addHeader(F("Content-Type"), contentType());
  #ifdef CONTROL_HTTPPOST_DEBUG
    Serial.print(id); Serial.print(F(": POST ")); Serial.print(url); Serial.print(' '); Serial.println(payload);
  #endif
  const int code = http.POST(payload);
  const bool ok = (code >= 200) && (code < 400); // A Google Apps Script answers with a redirect
  if (ok) {
    #ifdef CONTROL_HTTPPOST_DEBUG
      Serial.print(id); Serial.print(F(": HTTP ")); Serial.println(code);
    #endif
  } else {
    // Negative is HTTPClient's own error - -1 is "connection refused", i.e. nothing was sent
    Serial.print(id); Serial.print(F(": POST ")); Serial.print(url); Serial.print(F(" failed HTTP ")); Serial.println(code);
  }
  http.end();
  delete client;
  return ok;
}
