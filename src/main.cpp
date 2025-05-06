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
#ifdef SENSOR_ANALOG_INSTANCES_WANT
#include "sensor_analog_instances.h"
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
#ifdef SENSOR_BH1750_WANT
#include "sensor_bh1750.h"
#endif
#ifdef SENSOR_BUTTON_WANT
#include "sensor_button.h"
#endif
#ifdef SENSOR_MS5803_WANT
#include "sensor_ms5803.h"
#endif
#ifdef SENSOR_LOADCELL_WANT
#include "sensor_loadcell.h"
#endif
#ifdef CONTROL_BLINKEN_WANT
#include "control_blinken.h"
#endif
#ifdef CONTROL_WANT
#include "control.h"
#endif
#ifdef CONTROL_HYSTERISIS_WANT
#include "control_hysterisis.h"
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
#ifdef LOCAL_DEV_WANT
#include "local_dev.h"
#endif
void setup() {
#ifdef LILYGOHIGROW
  pinMode(POWER_CTRL, OUTPUT);
  digitalWrite(POWER_CTRL, HIGH); // TODO-115 this is for power control - may need other board specific stuff somewhere
#endif
#ifdef ANY_DEBUG
  Serial.begin(SERIAL_BAUD);
  while (!Serial) { 
    ; // wait for serial port to connect. Needed for Arduino Leonardo only
  }
  delay(SERIAL_DELAY); // If dont do this on D1 Mini and Arduino IDE then miss next debugging
  //Serial.setDebugOutput(true);  // Enable debug from wifi, also needed to enable output from printf
  Serial.println(F("FrugalIoT Starting"));
#endif // ANY_DEBUG
xWifi::setup();
Mqtt = new MqttManager(); // Connects to wifi and broker

//TO_ADD_ACTUATOR - follow the pattern below and add any variables and search for other places tagged TO_ADD_ACTUATOR
#ifdef ACTUATOR_LEDBUILTIN_WANT
  Actuator_Ledbuiltin* aLedBuiltin = new Actuator_Ledbuiltin(ACTUATOR_LEDBUILTIN_PIN, ACTUATOR_LEDBUILTIN_BRIGHTNESS, ACTUATOR_LEDBUILTIN_COLOR);
  actuators.push_back(aLedBuiltin);
#endif
#ifdef ACTUATOR_RELAY_WANT
  actuators.push_back(new Actuator_Digital("relay", "Relay", ACTUATOR_RELAY_PIN, "purple"));
#endif

// TODO_C++_EXPERT weird I have to assign to a vaiable even though constructor sticks in the "sensors" vector, else compiler complains, 
// but if I assign then it complaisn the variable isn't used.  And the pragma's seem to get ignored in the .ino file 
//TODO-25 try adding these to a Sensor* Vector here instead of in constructor
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#ifdef SENSOR_ANALOG_INSTANCES_WANT
  sensors.push_back(new Sensor_Analog("analog1", "Analog 1", SENSOR_ANALOG_PIN_1, SENSOR_ANALOG_SMOOTH_1, 0, 5000, SENSOR_ANALOG_COLOR_1, SENSOR_ANALOG_MS_1, true));
#endif
#ifdef SENSOR_ANALOG_PIN_2
  sensors.push_back(new Sensor_Analog("analog2", "Analog 2", SENSOR_ANALOG_PIN_2, SENSOR_ANALOG_SMOOTH_2, SENSOR_ANALOG_COLOR_2, SENSOR_ANALOG_MS_2, true));
#endif
#ifdef SENSOR_ANALOG_PIN_3
  sensors.push_back(new Sensor_Analog("analog3", "Analog 3", SENSOR_ANALOG_PIN_3, SENSOR_ANALOG_SMOOTH_3, SENSOR_ANALOG_COLOR_3, SENSOR_ANALOG_MS_3, true));
#endif
#ifdef SENSOR_ANALOG_PIN_4
  sensors.push_back(Sensor_Analog("analog4", "Analog 4", SENSOR_ANALOG_PIN_4, SENSOR_ANALOG_SMOOTH_4, SENSOR_ANALOG_COLOR_4, SENSOR_ANALOG_MS_4, true));
#endif
#ifdef SENSOR_ANALOG_PIN_5
  sensors.push_back(Sensor_Analog("analog5", "Analog 5", SENSOR_ANALOG_PIN_5, SENSOR_ANALOG_SMOOTH_5, SENSOR_ANALOG_COLOR_5, SENSOR_ANALOG_MS_5, true));
#endif

#ifdef SENSOR_BATTERY_WANT
  sensors.push_back(new Sensor_Battery(SENSOR_BATTERY_PIN));  // TODO-57 will rarely be as simple as this
#endif
#ifdef SENSOR_SHT_WANT
  sensors.push_back(new Sensor_SHT("SHT", SENSOR_SHT_ADDRESS, &Wire, SENSOR_SHT_MS, true));
#endif
#ifdef SENSOR_DHT_WANT
  sensors.push_back(new Sensor_DHT("DHT", SENSOR_DHT_PIN, SENSOR_DHT_MS, true));
#endif
#ifdef SENSOR_SOIL_WANT
  sensors.push_back(new Sensor_Soil("soil1", "Soil 1", SENSOR_SOIL_0, SENSOR_SOIL_100, SENSOR_SOIL_PIN, 0, "brown", SENSOR_SOIL_MS, true));
  #ifdef SENSOR_SOIL_PIN2
    sensors.push_back(new Sensor_Soil("soil2", "Soil 2", SENSOR_SOIL_0, SENSOR_SOIL_100, SENSOR_SOIL_PIN2, 0, "brown", SENSOR_SOIL_MS, true));
  #endif
  #ifdef SENSOR_SOIL_PIN3
    sensors.push_back(new Sensor_Soil("soil3", "Soil 3", SENSOR_SOIL_0, SENSOR_SOIL_100, SENSOR_SOIL_PIN3, 0, "brown", SENSOR_SOIL_MS, true));
  #endif
#endif
#ifdef SENSOR_BH1750_WANT
  sensors.push_back(new Sensor_BH1750(SENSOR_BH1750_ID, SENSOR_BH1750_NAME, SENSOR_BH1750_ADDRESS, SENSOR_BH1750_MS, true));
#endif
#ifdef SENSOR_MS5803_WANT
  sensors.push_back(new Sensor_ms5803("pressure", "Pressure"));
#endif
#ifdef SENSOR_BUTTON_WANT
  // Pushed to sensors by newSensor_Button
  Sensor_Button::newSensor_Button("button", "button", SENSOR_BUTTON_PIN);
#endif
#ifdef SENSOR_LOADCELL_WANT
  sensors.push_back(new Sensor_LoadCell("loadcell", "Load Cell", 2000, "pink", SENSOR_LOADCELL_MS, true));
#endif

xDiscovery::setup(); // Must be after system mqtt and before ACTUATOR* or SENSOR* or CONTROL* that setup topics

#ifdef CONTROL_BLINKEN_WANT
  Control* cb = new ControlBlinken("blinken", "Blinken", 5, 2);
  controls.push_back(cb);
  // TODO-130 make function an dredo wirepath
  cb->outputs[0]->wiredPath = Mqtt->path(aLedBuiltin->input->topicTwig); //TODO-25 turn into a function but note that aLedBuiltin will also change as gets INbool
#endif
#ifdef CONTROL_HYSTERISIS_WANT
// Example definition of control
  controls.push_back(new ControlHysterisis("humidity", "Humidity control", 50, 0, 100));
#endif //CONTROL_HYSTERISIS_WANT

#pragma GCC diagnostic pop


#ifdef SYSTEM_OTA_WANT
  // OTA should be after WiFi and before MQTT **but** it needs strings from Discovery TODO-37 fix this later - put strings somewhere global after WiFi
  xOta::setup();
#endif
#ifdef SYSTEM_TIME_WANT // Synchronize time
  xTime::setup();
#endif

#ifdef LOCAL_DEV_WANT
  localDev::setup(); // Note has to be before Frugal_Base::setupAll()
#endif

Frugal_Base::setupAll(); // Will replace all setups as developed - currently doing sensors and actuatorsand controls

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

#ifdef LOCAL_DEV_WANT
  localDev::loop();
#endif
}


