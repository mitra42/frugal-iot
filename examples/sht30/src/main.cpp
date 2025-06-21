/* 
 *  This is a test harness for the Frugal IoT project
 */

 // TODO-141 obsolete _settings.h from here - see legacyt
#include "_settings.h"  // For SERIAL_BAUD SERIAL_DELAY ANY_DEBUG

#define ANY_DEBUG
#if !defined(SERIAL_BAUD)
  #define SERIAL_BAUD 460800
#endif
#if !defined(SERIAL_DELAY)
  #define SERIAL_DELAY 5000
#endif


#include "frugal_iot.h"

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
  frugal_iot.sensors->add(new Sensor_SHT("SHT", SENSOR_SHT_ADDRESS, &Wire, true));
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop();
}

