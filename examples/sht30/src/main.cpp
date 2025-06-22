/* 
 *  Frugal IoT example - SHT30 temperature and humidity sensor
 * 
 * Optional: SENSOR_SHT_ADDRESS - defaults to 0x44, (note the D1 shields default to 0x45)
 */

 // TODO-141 obsolete _settings.h from here - see legacyt

#define ANY_DEBUG
#if !defined(SERIAL_BAUD)
  #define SERIAL_BAUD 460800
#endif
#if !defined(SERIAL_DELAY)
  #define SERIAL_DELAY 5000
#endif

// defines SENSOR_SHT_ADDRESS if dont define here or in platformio.ini
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
  frugal_iot.actuators->add(new Actuator_Ledbuiltin(ACTUATOR_LEDBUILTIN_PIN, ACTUATOR_LEDBUILTIN_BRIGHTNESS, ACTUATOR_LEDBUILTIN_COLOR));
  frugal_iot.setup(); // Has to be after setup sensors and actuators and controls
  Serial.println(F("FrugalIoT Starting Loop"));
}

void loop() {
  frugal_iot.loop();
}

