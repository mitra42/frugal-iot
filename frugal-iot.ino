/* 
 *  This is a test harness for the Frugal IoT project
 */

#include "_settings.h"  // Settings for what to include etc
#include "_common.h"    // Main include file for Framework

void setup() {
  while (!Serial) { 
    ; // wait for serial port to connect. Needed for Arduino Leonardo only
  }
#ifdef FRUGALIOT_DEBUGGING
  Serial.setDebugOutput(true);  // Enable debug from wifi, also needed to enable output from printf
  Serial.println("FrugalIoT Starting");
#endif FRUGALIOT_DEBUGGING

  // put setup code here, to run once:

#ifdef FRUGALIOT_DEBUGGING
  Serial.println("FrugalIoT Starting");
#endif FRUGALIOT_DEBUGGING
}

void loop() {
  // Put code for each sensor etc here - call functions in those sections
}


