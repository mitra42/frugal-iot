/* 
 *  This is a test harness for the Frugal IoT project
 */

#include "_settings.h"  // Settings for what to include etc

#include "_base.h" // Base for new class version
#include "sensor.h" // Base class for sensors
#include "misc.h" // 

#ifdef SYSTEM_WIFI_WANT
#include "system_wifi.h"
#endif
#ifdef SYSTEM_MQTT_WANT
#include "system_mqtt.h"
#endif
//TO_ADD_ACTUATOR - follow the pattern below and add any variables and search for other places tagged TO_ADD_ACTUATOR
#ifdef ACTUATOR_LEDBUILTIN_WANT
#include "actuator_ledbuiltin.h"
#endif
#ifdef ACTUATOR_RELAY_WANT
#include "actuator_relay.h"
#endif

// Follow the pattern below and add any variables and search for other places tagged TO_ADD_SENSOR
#ifdef SENSOR_ANALOG_EXAMPLE_WANT
#include "sensor_analog_example.h"
#endif
#ifdef SENSOR_SOIL_WANT
#include "sensor_soil.h"
#endif
#ifdef SENSOR_BATTERY_WANT
#include "sensor_battery.h"
#endif
#ifdef SENSOR_SHT_WANT
#include "sensor_sht.h"
#endif
#ifdef SENSOR_DHT_WANT
#include "sensor_dht.h"
#endif
#ifdef CONTROL_BLINKEN_WANT
#include "control_blinken.h"
#endif
#ifdef CONTROL_WANT
#include "control.h"
#endif
#ifdef SYSTEM_DISCOVERY_WANT
#include "system_discovery.h"
#endif
#ifdef SYSTEM_OTA_WANT
#include "system_ota.h"
#endif
#ifdef SYSTEM_TIME_WANT
#include "system_time.h"
#endif

#ifdef CONTROL_WANT

// TODO-25 move this function by making it a class in control_hysterisis
Control::TCallback hysterisisAction = [](Control* self) {
    const float hum = self->inputs[0]->floatValue();
    const float lim = self->inputs[1]->floatValue();
    const float hysterisis = self->inputs[2]->floatValue();
    if (hum > (lim + hysterisis)) {
        self->outputs[0]->set(1);
    }
    if (hum < (lim - hysterisis)) {
        self->outputs[0]->set(0);
    }
  // If  lim-histerisis < hum < lim+histerisis then don't change setting
};
#endif //CONTROL_WANT
void setup() {
#ifdef ANY_DEBUG
  Serial.begin(SERIAL_BAUD);
  while (!Serial) { 
    ; // wait for serial port to connect. Needed for Arduino Leonardo only
  }
  delay(SERIAL_DELAY); // If dont do this on D1 Mini and Arduino IDE then miss next debugging
  //Serial.setDebugOutput(true);  // Enable debug from wifi, also needed to enable output from printf
  Serial.println(F("FrugalIoT Starting"));
#endif // ANY_DEBUG
// put setup code here, to run once:
xWifi::setup();
Mqtt = new MqttManager();


//TO_ADD_ACTUATOR - follow the pattern below and add any variables and search for other places tagged TO_ADD_ACTUATOR
#ifdef ACTUATOR_LEDBUILTIN_WANT
Actuator_Ledbuiltin* a1 = new Actuator_Ledbuiltin(ACTUATOR_LEDBUILTIN_PIN, "ledbuiltin");
#endif
#ifdef ACTUATOR_RELAY_WANT
Actuator_Digital* a2 = new Actuator_Digital(ACTUATOR_RELAY_PIN, "relay");
#endif

// TODO_C++_EXPERT weird I have to assign to a vaiable even though constructor sticks in the "sensors" vector, else compiler complains, 
// but if I assign then it complaisn the variable isn't used.  And the pragma's seem to get ignored in the .ino file 
//TODO-25 try adding these to a Sensor* Vector here instead of in constructor
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#ifdef SENSOR_ANALOG_EXAMPLE_WANT
  Sensor_Analog* s3 = new Sensor_Analog(SENSOR_ANALOG_EXAMPLE_PIN, SENSOR_ANALOG_EXAMPLE_SMOOTH, SENSOR_ANALOG_EXAMPLE_TOPIC, SENSOR_ANALOG_EXAMPLE_MS);
#endif
#ifdef SENSOR_BATTERY_WANT
  Sensor_Battery* s4 = new Sensor_Battery(SENSOR_BATTERY_PIN);  // TODO-57 will rarely be as simple as this
#endif
#ifdef SENSOR_SHT_WANT
  Sensor_SHT* s1 = new Sensor_SHT(SENSOR_SHT_ADDRESS, &Wire, "temperature", "humidity", SENSOR_SHT_MS);
#endif
#ifdef SENSOR_DHT_WANT
  Sensor_DHT* s2 = new Sensor_DHT(SENSOR_DHT_PIN, "temperature", "humidity", SENSOR_DHT_MS);
#endif
#ifdef SENSOR_SOIL_WANT
  Sensor_Soil* s5a = new Sensor_Soil(SENSOR_SOIL_0, SENSOR_SOIL_100, SENSOR_SOIL_PIN, 0, SENSOR_SOIL_TOPIC, SENSOR_SOIL_MS);
  #ifdef SENSOR_SOIL_PIN2
    Sensor_Soil* s5b = new Sensor_Soil(SENSOR_SOIL_0, SENSOR_SOIL_100, SENSOR_SOIL_PIN2, 0, SENSOR_SOIL_TOPIC "2", SENSOR_SOIL_MS);
  #endif
  #ifdef SENSOR_SOIL_PIN3
    Sensor_Soil* s5c = new Sensor_Soil(SENSOR_SOIL_0, SENSOR_SOIL_100, SENSOR_SOIL_PIN3, 0, SENSOR_SOIL_TOPIC "3", SENSOR_SOIL_MS);
  #endif
#endif

#ifdef CONTROL_WANT
// Example definition of control - //TODO-25 move this to control_histerisis, but make sure run in setup, not prior to it as need Mqtt

Control* control_humidity = new Control(
  "humidity_control",
  std::vector<IN*> {
    new INfloat("humiditynow", 0.0, "humiditynow", 0, 100, "blue", true),
    new INfloat("limit", 50.0, "humidity_limit", 0, 100, "red", false),
    new INfloat("hysterisis", 5.0, "hysterisis", 0, 20, "purple", false) // Note nullptr needed in .ino but not .cpp files :-(
    },
  std::vector<OUT*> {
    new OUTbool("out", false, "too_humid", "blue", true), // Default to control LED, controllable via "relay_control")
  },
  std::vector<Control::TCallback> {
    hysterisisAction
  });

  control_humidity->debug("frugal-iot.ino after instantiation");
#endif //CONTROL_WANT
#pragma GCC diagnostic pop

xDiscovery::setup(); // Must be after system mqtt and before ACTUATOR* or SENSOR* or CONTROL* that setup topics
#ifdef SYSTEM_OTA_WANT
  // OTA should be after WiFi and before MQTT **but** it needs strings from Discovery TODO-37 fix this later - put strings somewhere global after WiFi
  xOta::setup();
#endif
#ifdef SYSTEM_TIME_WANT // Synchronize time
  xTime::setup();
#endif



Frugal_Base::setupAll(); // Will replace all setups as developed - currently doing sensors and actuatorsand controls

#ifdef CONTROL_BLINKEN_WANT
  cBlinken::setup();
#endif
   // Tell broker what I've got at start (has to be before quickAdvertise; after sensor & actuator*::setup so can't be inside xDiscoverSetup
  xDiscovery::fullAdvertise();

  // TODO-125 want to ifdef this
  internal_watchdog_setup();

#ifdef ANY_DEBUG
  Serial.println(F("FrugalIoT Starting Loop"));
#endif // ANY_DEBUG

}

void loop() {
  Mqtt->loop();
  Frugal_Base::loopAll(); // Will replace all loops as developed - but starting with sensors, so positioned here.

#ifdef CONTROL_BLINKEN_WANT
  cBlinken::loop();
#endif
#ifdef SYSTEM_DISCOVERY_WANT
  xDiscovery::loop();
#endif
#ifdef SYSTEM_OTA_WANT
  xOta::loop();
#endif
#ifdef SYSTEM_TIME_WANT
  xTime::loop();
#endif
  // TODO-125 probably want to ifdef this
  internal_watchdog_loop();
}


