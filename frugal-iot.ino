/* 
 *  This is a test harness for the Frugal IoT project
 */

#include "_settings.h"  // Settings for what to include etc

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

//TO_ADD_SENSOR - follow the pattern below and add any variables and search for other places tagged TO_ADD_SENSOR
#ifdef SENSOR_ANALOG_WANT
#include "sensor_analog.h"
#endif
#ifdef SENSOR_SHT85_WANT
#include "sensor_sht85.h"
#endif
#ifdef CONTROL_BLINKEN_WANT
#include "control_blinken.h"
#endif
#ifdef CONTROL_DEMO_MQTT_WANT
#include "control_demo_mqtt.h"
#endif
#ifdef SYSTEM_DISCOVERY_WANT
#include "system_discovery.h"
#endif

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
#ifdef SYSTEM_WIFI_WANT
  xWifi::setup();
#endif
#ifdef SYSTEM_MQTT_WANT
  xMqtt::setup();
#endif
#ifdef SYSTEM_DISCOVERY_WANT
  xDiscovery::setup(); // Must be after system mqtt and before ACTUATOR* or SENSOR* or CONTROL* that setup topics
#endif
//TO_ADD_ACTUATOR - follow the pattern below and add any variables and search for other places tagged TO_ADD_ACTUATOR
#ifdef ACTUATOR_LEDBUILTIN_WANT
  aLedbuiltin::setup();
#endif
#ifdef ACTUATOR_RELAY_WANT
  aRelay::setup();
#endif

//TO_ADD_SENSOR - follow the pattern below and add any variables and search for other places tagged TO_ADD_SENSOR
#ifdef SENSOR_ANALOG_WANT
  sAnalog::setup();
#endif
#ifdef SENSOR_SHT85_WANT
  sSHT85::setup();
#endif
#ifdef CONTROL_BLINKEN_WANT
  cBlinken::setup();
#endif
#ifdef CONTROL_DEMO_MQTT_WANT
  cDemoMqtt::setup(); // Must be after system_mqtt
#endif

#ifdef ANY_DEBUG
  Serial.println(F("FrugalIoT Starting Loop"));
#endif // ANY_DEBUG

}

void loop() {
  // Put code for each sensor etc here - call functions in those sections
#ifdef SYSTEM_MQTT_WANT
  xMqtt::loop();
#endif
//TO_ADD_ACTUATOR - follow the pattern below and add any variables and search for other places tagged TO_ADD_ACTUATOR
/*
#ifdef ACTUATOR_XXX
  aXXX::loop();
#endif
*/

//TO_ADD_SENSOR - follow the pattern below and add any variables and search for other places tagged TO_ADD_SENSOR
#ifdef SENSOR_ANALOG_WANT
  sAnalog::loop();
#endif
#ifdef SENSOR_SHT85_WANT
  sSHT85::loop();
#endif
#ifdef CONTROL_BLINKEN_WANT
  cBlinken::loop();
#endif
#ifdef SYSTEM_DISCOVERY_WANT
  xDiscovery::loop();
#endif
}


