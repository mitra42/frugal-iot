/* 
 *  This is a test harness for the Frugal IoT project
 */

#include "_settings.h"  // Settings for what to include etc

#include "_base.h" // Base for new class version
#include "sensor.h" // Base class for sensors
#include "misc.h" // 
#include "main.h"

#ifdef SYSTEM_WIFI_WANT
#include "system_wifi.h"
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
#ifdef SENSOR_ENSAHT_WANT
#include "sensor_ens160aht21.h"
#endif
#ifdef CONTROL_BLINKEN_WANT
#include "control_blinken.h"
#endif
#ifdef CONTROL_GSHEETS_WANT
#include "control_gsheets.h"
#endif
#ifdef CONTROL_WANT
#include "control.h"
#endif
#ifdef CONTROL_HYSTERISIS_WANT
#include "control_hysterisis.h"
#endif
#ifdef CONTROL_LOGGERFS_WANT
#include "control_logger_fs.h"
#include "system_fs.h"
#endif

#ifdef LOCAL_DEV_WANT
#include "local_dev.h"
#endif

#include "frugal_iot.h"

Frugal_IoT frugal_iot; // Singleton

void setup() {

Serial.print("Setup: ");
frugal_iot.setup(); // TODO-141 move most of below into this and this should come after sensors added.
Serial.println();

#ifdef LILYGOHIGROW // TODO-141 maybe move to "boards"
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

//TO_ADD_ACTUATOR - follow the pattern below and add any variables and search for other places tagged TO_ADD_ACTUATOR
#ifdef ACTUATOR_LEDBUILTIN_WANT
  frugal_iot.actuators->add(new Actuator_Ledbuiltin(ACTUATOR_LEDBUILTIN_PIN, ACTUATOR_LEDBUILTIN_BRIGHTNESS, ACTUATOR_LEDBUILTIN_COLOR));
#endif
#ifdef ACTUATOR_RELAY_WANT
  frugal_iot.actuators->add(new Actuator_Digital("relay", "Relay", ACTUATOR_RELAY_PIN, "purple"));
#endif

#ifdef SENSOR_ANALOG_INSTANCES_WANT
  frugal_iot.sensors->add(new Sensor_Analog("analog1", "Analog 1", SENSOR_ANALOG_PIN_1, SENSOR_ANALOG_SMOOTH_1, 0, 5000, SENSOR_ANALOG_COLOR_1, SENSOR_ANALOG_MS_1, true));
#endif
#ifdef SENSOR_ANALOG_PIN_2
  frugal_iot.sensors->add(new Sensor_Analog("analog2", "Analog 2", SENSOR_ANALOG_PIN_2, SENSOR_ANALOG_SMOOTH_2, SENSOR_ANALOG_COLOR_2, SENSOR_ANALOG_MS_2, true));
#endif
#ifdef SENSOR_ANALOG_PIN_3
  frugal_iot.sensors->add(new Sensor_Analog("analog3", "Analog 3", SENSOR_ANALOG_PIN_3, SENSOR_ANALOG_SMOOTH_3, SENSOR_ANALOG_COLOR_3, SENSOR_ANALOG_MS_3, true));
#endif
#ifdef SENSOR_ANALOG_PIN_4
  frugal_iot.sensors->add(Sensor_Analog("analog4", "Analog 4", SENSOR_ANALOG_PIN_4, SENSOR_ANALOG_SMOOTH_4, SENSOR_ANALOG_COLOR_4, SENSOR_ANALOG_MS_4, true));
#endif
#ifdef SENSOR_ANALOG_PIN_5
  frugal_iot.sensors->add(Sensor_Analog("analog5", "Analog 5", SENSOR_ANALOG_PIN_5, SENSOR_ANALOG_SMOOTH_5, SENSOR_ANALOG_COLOR_5, SENSOR_ANALOG_MS_5, true));
#endif

#ifdef SENSOR_BATTERY_WANT
  frugal_iot.sensors->add(new Sensor_Battery(SENSOR_BATTERY_PIN));  // TODO-57 will rarely be as simple as this
#endif
#ifdef SENSOR_SHT_WANT
  Sensor_SHT* ss = new Sensor_SHT("SHT", SENSOR_SHT_ADDRESS, &Wire, SENSOR_SHT_MS, true);
  frugal_iot.sensors->add(ss);
#endif
#ifdef SENSOR_DHT_WANT
  frugal_iot.sensors->add(new Sensor_DHT("DHT", SENSOR_DHT_PIN, SENSOR_DHT_MS, true));
#endif
#ifdef SENSOR_SOIL_WANT
  frugal_iot.sensors->add(new Sensor_Soil("soil1", "Soil 1", SENSOR_SOIL_0, SENSOR_SOIL_100, SENSOR_SOIL_PIN, 0, "brown", SENSOR_SOIL_MS, true));
  #ifdef SENSOR_SOIL_PIN2
    frugal_iot.sensors->add(new Sensor_Soil("soil2", "Soil 2", SENSOR_SOIL_0, SENSOR_SOIL_100, SENSOR_SOIL_PIN2, 0, "brown", SENSOR_SOIL_MS, true));
  #endif
  #ifdef SENSOR_SOIL_PIN3
    frugal_iot.sensors->add(new Sensor_Soil("soil3", "Soil 3", SENSOR_SOIL_0, SENSOR_SOIL_100, SENSOR_SOIL_PIN3, 0, "brown", SENSOR_SOIL_MS, true));
  #endif
#endif
#ifdef SENSOR_BH1750_WANT
  frugal_iot.sensors->add(new Sensor_BH1750(SENSOR_BH1750_ID, SENSOR_BH1750_NAME, SENSOR_BH1750_ADDRESS, SENSOR_BH1750_MS, true));
#endif
#ifdef SENSOR_MS5803_WANT
  frugal_iot.sensors->add(new Sensor_ms5803("pressure", "Pressure"));
#endif
#ifdef SENSOR_BUTTON_WANT
  frugal_iot.sensors->add(new Sensor_Button("button", "Button", SENSOR_BUTTON_PIN, "purple"));
#endif
#ifdef SENSOR_LOADCELL_WANT
  frugal_iot.sensors->add(new Sensor_LoadCell("loadcell", "Load Cell", 2000, "pink", SENSOR_LOADCELL_MS, true));
#endif
#ifdef SENSOR_ENSAHT_WANT
  frugal_iot.sensors->add(new Sensor_ensaht("ensaht","ENS AHT"));
#endif

#ifdef CONTROL_BLINKEN_WANT
  // TODO-141 figure out how cb used 
  Control* cb = new ControlBlinken("blinken", "Blinken", 5, 2);
  frugal_iot.controls->add(cb);
  cb->outputs[0]->wireTo(frugal_iot.mqtt->path("ledbuiltin/id")); //TODO-25 turn into a function but note that aLedBuiltin will also change as gets INbool
#endif
#ifdef CONTROL_HYSTERISIS_WANT
// Example definition of control
  frugal_iot.controls->add(new ControlHysterisis("control", "Control", 50, 1, 0, 100));
#endif //CONTROL_HYSTERISIS_WANT
#ifdef CONTROL_GSHEETS_WANT
  // TODO-141 figure out how cg used 
  Control_Gsheets* cg =   new Control_Gsheets("gsheets demo", CONTROL_GSHEETS_URL);
  frugal_iot.controls->add(cg);
  cg->track("temperature", frugal_iot.mqtt->path("sht/temperature"));
#endif

#ifdef SYSTEM_SD_WANT
  System_SD* fs_SD = new System_SD();
  frugal_iot.system->add(fs_SD);
#endif
#ifdef SYSTEM_SPIFFS_WANT
  System_SPIFFS* fs_SPIFFS = new System_SPIFFS();
  frugal_iot.system->add(fs_SPIFFS);
#endif

#ifdef CONTROL_LOGGERFS_WANT
// Must be after sensor_sht for default wiring below
// TODO-141 Make match pattern
Control_Logger* clfs = new Control_LoggerFS(
  "Logger",
  fs_SPIFFS, // TODO-110 Using spiffs for testing for now
  "/",
  0x02, // Single log.csv with topicPath, time, value
  std::vector<IN*> {
    //INtext(const char * const sensorId, const char * const id, const char* const name, String* value, const char* const color, const bool wireable)
    new INtext("Logger", "log1", "log1", nullptr, "black", true),
    new INtext("Logger", "log2", "log2", nullptr, "black", true),
    new INtext("Logger", "log3", "log3", nullptr, "black", true)
    });
  frugal_iot.controls->add(clfs);
  clfs->inputs[0]->wireTo(ss->temperature); // TODO this is default wiring - should remove.
#endif // CONTROL_LOGGERFS_WANT

#ifdef LOCAL_DEV_WANT
  // TODO-141 move into frugal_iot. 
  localDev::setup(); // Note has to be before Frugal_Base::setupAll() TODO-141 rework this, e.g. push the local
#endif

#ifdef ANY_DEBUG  
  Serial.println(F("FrugalIoT Starting Loop"));
#endif // ANY_DEBUG

}

bool donePeriodic = false;
void loop() {
// TODO-141 move into frugal_iot. 
  if (!donePeriodic) {
    frugal_iot.periodically();  // Do things run once per cycle
    frugal_iot.infrequently();  // Do things that keep their own track of time
    donePeriodic = true;
  }
  frugal_iot.frequently(); // Do things like MQTT which run frequently with their own clock
  if (frugal_iot.powercontroller->maybeSleep()) { // Note this returns true if sleep, OR if period for POWER_MODE_LOOP
    donePeriodic = false; // reset after sleep (note deep sleep comes in at top again)
  }
}


