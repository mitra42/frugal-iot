/* 
 *  This is a test harness for the Frugal IoT project
 */

#include "_settings.h"  // Settings for what to include etc
#include "_common.h"    // Main include file for Framework

void setup() {
#ifdef FRUGALIOT_DEBUG
  Serial.begin(57600); // Initialize IO port TODO move to somewhere Forth wants it
  while (!Serial) { 
    ; // wait for serial port to connect. Needed for Arduino Leonardo only
  }
  //Serial.setDebugOutput(true);  // Enable debug from wifi, also needed to enable output from printf
  Serial.println("FrugalIoT Starting");
#endif FRUGALIOT_DEBUG
// put setup code here, to run once:
#ifdef WANT_ACTUATOR_BLINKEN
  aBlinken::setup();
#endif

#ifdef FRUGALIOT_DEBUG
  Serial.println("FrugalIoT Starting Loop");
#endif FRUGALIOT_DEBUG
}

void loop() {
  // Put code for each sensor etc here - call functions in those sections
#ifdef WANT_ACTUATOR_BLINKEN
  aBlinken::loop();
#endif
}


