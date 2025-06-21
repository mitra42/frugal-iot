/* 
 *  This is a test harness for the Frugal IoT project
 */

 // TODO-141 obsolete _settings.h from here - see legacyt
#include "_settings.h"  // For SERIAL_BAUD SERIAL_DELAY ANY_DEBUG

#include "frugal_iot.h"

// TODO-141 move this into frugal_iot
#ifdef LOCAL_DEV_WANT
#include "local_dev.h"
#endif

System_Frugal frugal_iot; // Singleton

void setup() {

  #ifdef ANY_DEBUG
    Serial.begin(SERIAL_BAUD);
    while (!Serial) { 
      ; // wait for serial port to connect. Needed for Arduino Leonardo only
    }
    delay(SERIAL_DELAY); // If dont do this on D1 Mini and Arduino IDE then miss next debugging
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

frugal_iot.setup(); // Has to be after setup sensors and actuators and controls

#ifdef ANY_DEBUG  
  Serial.println(F("FrugalIoT Starting Loop"));
#endif // ANY_DEBUG

}

void loop() {
  frugal_iot.loop();
}

