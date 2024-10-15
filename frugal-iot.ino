/* 
 *  This is a test harness for the Frugal IoT project
 */

#include "_settings.h"  // Settings for what to include etc
#include "_common.h"    // Main include file for Framework

#ifdef SYSTEM_WIFI_WANT
#include "system_wifi.h"
#endif
#ifdef SYSTEM_MQTT_WANT
#include "system_mqtt.h"
#endif
#ifdef ACTUATOR_LEDBUILTIN_WANT
#include "actuator_ledbuiltin.h"
#endif
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

void setup() {
#ifdef FRUGALIOT_DEBUG
  Serial.begin(460800); // Initialize IO port TODO move to somewhere Forth wants it
  while (!Serial) { 
    ; // wait for serial port to connect. Needed for Arduino Leonardo only
  }
  delay(5000); // If dont do this on D1 Mini and Arduino IDE then miss next debugging
  //Serial.setDebugOutput(true);  // Enable debug from wifi, also needed to enable output from printf
  Serial.println("FrugalIoT Starting");
#endif FRUGALIOT_DEBUG
// put setup code here, to run once:
#ifdef SYSTEM_WIFI_WANT
  xWifi::setup();
#endif
#ifdef SYSTEM_MQTT_WANT
  xMqtt::setup();
#endif
#ifdef ACTUATOR_LEDBUILTIN_WANT
  aLedbuiltin::setup();
#endif
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

#ifdef FRUGALIOT_DEBUG
  Serial.println("FrugalIoT Starting Loop");
#endif FRUGALIOT_DEBUG
}

void loop() {
  // Put code for each sensor etc here - call functions in those sections
#ifdef SYSTEM_WIFI_WANT
//  xWifi::loop(); // Not currently used
#endif
#ifdef SYSTEM_MQTT_WANT
  xMqtt::loop();
#endif
#ifdef ACTUATOR_LEDBUILTIN_WANT
  aLedbuiltin::loop();
#endif
#ifdef SENSOR_ANALOG_WANT
  sAnalog::loop();
#endif
#ifdef SENSOR_SHT85_WANT
  sSHT85::loop();
#endif
#ifdef CONTROL_BLINKEN_WANT
  cBlinken::loop();
#endif
#ifdef CONTROL_DEMO_MQTT_WANT
//  cDemoMqtt::loop(); // Not currently used
#endif

}


