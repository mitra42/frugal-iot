/* A sensor wired to a Control_Hysteresis, driven entirely by messages.
 *
 *   fake (Sensor_Fake) --wired--> controlhysteresis/now ; controlhysteresis/out is published
 *
 * The tests run in order against one node, and each sets up the state it depends on rather than
 * relying on the test before it.
 */
#include "FrugalTest.h"

System_Frugal frugal_iot("test", "test", "Test", "Frugal-IoT test node");
Sensor_Fake* fake;
Control_Hysteresis* hyst;

void setUp() { clearOutgoing(); }
void tearDown() { }

// Set the input, take a reading, and let it run through the wiring
static void reading(float v) {
  fake->next = v;
  fake->read();
  pump();
}

void test_reading_is_published_retained() {
  reading(21.5);
  const System_Message* m = published("fake/fake");
  TEST_ASSERT_NOT_NULL(m);
  TEST_ASSERT_EQUAL_STRING("21.5", m->payload.c_str());
  TEST_ASSERT_TRUE(m->retain);
}

void test_unchanged_reading_is_not_republished() {
  reading(22.0);
  clearOutgoing();
  reading(22.0);
  TEST_ASSERT_NULL(published("fake/fake"));
}

void test_no_reading_publishes_nan_once() {
  reading(NAN);
  TEST_ASSERT_EQUAL_STRING(IO_PAYLOAD_INVALID, latest("fake/fake").c_str());
  clearOutgoing();
  reading(NAN); // NaN != NaN, so a plain != would publish again
  TEST_ASSERT_NULL(published("fake/fake"));
}

/* Wired to the local sensor, limit 20, output off, and nothing published yet.
 *
 * Every test using the output starts here, because an output only publishes when it CHANGES -
 * a test expecting "1" from a node whose output was left on by an earlier test sees nothing.
 */
static void startOff() {
  inject("set/controlhysteresis/now/wired", frugal_iot.messages->path("fake/fake"));
  inject("set/controlhysteresis/limit", "20");
  pump();
  reading(10);
  TEST_ASSERT_EQUAL_STRING("0", statusValue("controlhysteresis/out").c_str());
  clearOutgoing();
}

void test_hysteresis_switches_on_and_off() {
  startOff();
  reading(25);
  TEST_ASSERT_EQUAL_STRING("1", latest("controlhysteresis/out").c_str());
  reading(15);
  TEST_ASSERT_EQUAL_STRING("0", latest("controlhysteresis/out").c_str());
}

void test_hysteresis_holds_when_input_has_no_reading() {
  startOff();
  reading(25);
  TEST_ASSERT_EQUAL_STRING("1", latest("controlhysteresis/out").c_str());
  clearOutgoing();
  reading(NAN);
  TEST_ASSERT_EQUAL_STRING(IO_PAYLOAD_INVALID, latest("fake/fake").c_str());
  TEST_ASSERT_NULL(published("controlhysteresis/out")); // Held at 1, so nothing sent
}

void test_set_limit_is_echoed_and_persisted() {
  inject("set/controlhysteresis/limit", "30");
  pump();
  TEST_ASSERT_EQUAL_STRING("30", latest("controlhysteresis/limit").c_str());
  TEST_ASSERT_TRUE(persisted("controlhysteresis", "limit/value"));
}

void test_status_marks_persisted_limit() {
  inject("set/controlhysteresis/limit", "30");
  pump();
  TEST_ASSERT_CONTAINS("controlhysteresis/limit 30.0 *", statusText());
}

void test_rewire_to_another_node() {
  const String remote = String("test/test/esp32-remote/sht/temperature");
  startOff();
  inject("set/controlhysteresis/now/wired", remote);
  pump();
  TEST_ASSERT_TRUE(subscribed(remote));
  TEST_ASSERT_EQUAL_STRING(remote.c_str(), latest("controlhysteresis/now/wired").c_str());
  TEST_ASSERT_TRUE(persisted("controlhysteresis", "now/wired"));
  injectPath(remote, "25");
  pump();
  TEST_ASSERT_EQUAL_STRING("1", latest("controlhysteresis/out").c_str());
  injectPath(remote, "15");
  pump();
  TEST_ASSERT_EQUAL_STRING("0", latest("controlhysteresis/out").c_str());
  // The local sensor is no longer what drives it
  clearOutgoing();
  reading(10);
  TEST_ASSERT_NULL(published("controlhysteresis/out"));
}

void setup() {
  delay(2000); // Let the test runner open the serial port
  UNITY_BEGIN();
  frugalTestPreSetup();
  fake = new Sensor_Fake("fake", "Fake");
  frugal_iot.sensors->add(fake);
  hyst = new Control_Hysteresis("controlhysteresis", "Control", 20, 1, 0, 100);
  frugal_iot.controls->add(hyst);
  hyst->wireFrom(fake);
  frugal_iot.setup();
  pump();

  RUN_TEST(test_reading_is_published_retained);
  RUN_TEST(test_unchanged_reading_is_not_republished);
  RUN_TEST(test_no_reading_publishes_nan_once);
  RUN_TEST(test_hysteresis_switches_on_and_off);
  RUN_TEST(test_hysteresis_holds_when_input_has_no_reading);
  RUN_TEST(test_set_limit_is_echoed_and_persisted);
  RUN_TEST(test_status_marks_persisted_limit);
  RUN_TEST(test_rewire_to_another_node);
  UNITY_END();
}

void loop() {
  frugal_iot.loop(); // Results are in; just keep the node alive, as an idle loop() would trip the watchdog
}
