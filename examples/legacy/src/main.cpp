/* 
 *  This is a test harness for the Frugal IoT project
 */

 // TODO-141 obsolete _settings.h from here - see legacyt
#include "_settings.h"  // For SERIAL_BAUD SERIAL_DELAY ANY_DEBUG

#include "frugal_iot.h"

System_Frugal frugal_iot; // Singleton

void setup() {


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

