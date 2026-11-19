/* Frugal-IoT test helpers - shared by every suite in test/
 *
 * A suite is one node: it builds a System_Frugal with whatever modules it is testing, and then
 * talks to it only through messages - feeding them in with inject() as MQTT would, and reading
 * what came out of System_Messages::outgoing. No broker is connected, so nothing ever leaves that
 * queue, and sendRemote() keeps only the latest payload for each topic.
 *
 * Readings come from Sensor_Fake, whose value the test sets. Nothing here touches real hardware.
 */
#ifndef FRUGAL_TEST_H
#define FRUGAL_TEST_H

#include <Arduino.h>
#include <LittleFS.h>
#include <unity.h>
#include "Frugal-IoT.h"

// A float sensor whose next reading is whatever the test says, NAN for "no reading"
class Sensor_Fake : public Sensor_Float {
  public:
    float next = NAN;
    Sensor_Fake(const char* const id, const char* const name)
    : Sensor_Float(id, name, 1, -100, 100, "black", true) { }
    // What periodically() does each cycle, on demand
    void read() { readValidateConvertSet(); }
  protected:
    float readFloat() override { return next; }
};

// Every suite starts from an empty filesystem, so nothing persisted by an earlier run leaks in
inline void frugalTestFormat() {
  #ifdef ESP32
    LittleFS.begin(true);
  #else
    LittleFS.begin();
  #endif
  LittleFS.format();
  LittleFS.end();
}

// Everything that comes before a suite adds its modules. Power_Loop with an hour's cycle, so
// periodically() runs once on the first loop() and the tests then drive reads themselves.
// MQTT is configured because messages need an mqtt to ask whether it is connected; it never is.
inline void frugalTestPreSetup() {
  frugalTestFormat();
  frugal_iot.configure_power(Power_Loop, 3600000, 3600000);
  frugal_iot.pre_setup();
  frugal_iot.configure_mqtt("mqtt.invalid", "test", "test");
}

// Run the node for a few passes, so queued messages are dispatched and whatever they cause is
// dispatched in turn - a wired chain takes one pass per hop
inline void pump(uint8_t passes = 10) {
  for (uint8_t i = 0; i < passes; i++) {
    frugal_iot.loop();
  }
}

// Forget everything published so far, so a test can ask whether something was published at all
inline void clearOutgoing() {
  frugal_iot.messages->outgoing.clear();
}

// The latest message published on this node's twig, e.g. "sht/temperature", or nullptr
inline const System_Message* published(const String& twig) {
  const String topicPath = frugal_iot.messages->path(twig);
  const System_Message* found = nullptr;
  for (System_Message& m : frugal_iot.messages->outgoing) {
    if (!m.isSubscription() && (m.topicPath == topicPath)) {
      found = &m;
    }
  }
  return found;
}

// Its payload, or "<none>" - readable in a failed TEST_ASSERT_EQUAL_STRING
inline String latest(const String& twig) {
  const System_Message* m = published(twig);
  return m ? m->payload : String("<none>");
}

// Was a subscription to this full topic path sent upstream?
inline bool subscribed(const String& topicPath) {
  bool found = false;
  for (System_Message& m : frugal_iot.messages->outgoing) {
    if (m.isSubscription() && (m.topicPath == topicPath)) {
      found = true;
    }
  }
  return found;
}

// A message arriving from the broker for this node, e.g. inject("set/sht/temperature/max", "40")
inline void inject(const String& twig, const String& payload) {
  frugal_iot.messages->queueIncoming(frugal_iot.messages->path(twig), payload, MsgFromMQTT);
}
// A message arriving from the broker on any topic, e.g. another node's reading
inline void injectPath(const String& topicPath, const String& payload) {
  frugal_iot.messages->queueIncoming(topicPath, payload, MsgFromMQTT);
}

// Is this module's setting on the filesystem, so it survives a restart?
inline bool persisted(const String& module, const String& leaf) {
  return frugal_iot.fs_LittleFS->exists(frugal_iot.fs_LittleFS->configPath(module, leaf));
}

// The /status page as text
class StringPrint : public Print {
  public:
    String text;
    size_t write(uint8_t c) override { text += (char)c; return 1; }
};
inline String statusText(bool full = false) {
  StringPrint out;
  frugal_iot.statusLines(&out, full);
  return out.text;
}

// The current value of an IO, e.g. statusValue("sht/temperature") - from the status page, so it
// is the value held now, whether or not anything has been published since clearOutgoing()
inline String statusValue(const String& twig) {
  const String text = statusText();
  const String key = String("\n") + twig + " ";
  String value = "<none>";
  const int start = text.indexOf(key);
  if (start >= 0) {
    const int from = start + key.length();
    int to = from;
    while ((to < (int)text.length()) && (text.charAt(to) != ' ') && (text.charAt(to) != '\n') && (text.charAt(to) != '\r')) {
      to++;
    }
    value = text.substring(from, to);
  }
  return value;
}

// TEST_ASSERT on a String containing another, printing both if it does not
#define TEST_ASSERT_CONTAINS(needle, haystack) \
  TEST_ASSERT_MESSAGE(String(haystack).indexOf(needle) >= 0, (String("expected \"") + (needle) + "\" in:\n" + (haystack)).c_str())

#endif // FRUGAL_TEST_H
