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

  Sensor_SHT* ss = new Sensor_SHT("SHT", SENSOR_SHT_ADDRESS, &Wire, true);
  frugal_iot.sensors->add(ss);
#ifdef SENSOR_MS5803_WANT
  frugal_iot.sensors->add(new Sensor_ms5803("pressure", "Pressure"));
#endif
#ifdef SENSOR_LOADCELL_WANT
  frugal_iot.sensors->add(new Sensor_LoadCell("loadcell", "Load Cell", 2000, "pink", true));
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
#ifdef CONTROL_GSHEETS_WANT
  // TODO-141 figure out how cg used 
  Control_Gsheets* cg =   new Control_Gsheets("gsheets demo", CONTROL_GSHEETS_URL);
  frugal_iot.controls->add(cg);
  cg->track("temperature", frugal_iot.mqtt->path("sht/temperature"));
#endif

frugal_iot.setup(); // Has to be after setup sensors and actuators and controls

#ifdef ANY_DEBUG  
  Serial.println(F("FrugalIoT Starting Loop"));
#endif // ANY_DEBUG

}

void loop() {
  frugal_iot.loop();
}

