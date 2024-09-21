/*
 * Temperature and Humidity sensor, 
 * Based on SHT85_demo_async.ino in https://github.com/RobTillaart/SHT85
 *
 * Tested on Lolin SHT30 shield
 */

#include "_settings.h"  // Settings for what to include etc
#include "_common.h"    // Main include file for Framework

#ifdef WANT_SENSOR_SHT85

#include <Arduino.h>
#include <SHT85.h>
#include "system_clock.h"
#include "sensor_sht85.h"


namespace sSHT85 {

SENSOR_SHT85_DEVICE sht(SENSOR_SHT85_ADDRESS);
unsigned long lastLoopTime = 0;
float temperature; 
float humidity; 

void setup()
{
#ifdef SENSOR_SHT85_DEBUG
  Serial.println(__FILE__);
  Serial.print("SHT_LIB_VERSION: \t");
  Serial.print(SHT_LIB_VERSION);
  Serial.print(" on "); 
  Serial.print(SENSOR_SHT85_ADDRESS, HEX);
#endif

  //TODO It might be that we have to be careful to only setup the Wire once if there are multiple sensors. 
  Wire.begin();
  Wire.setClock(100000);

  sht.begin();
  uint16_t stat = sht.readStatus();

#ifdef SENSOR_SHT85_DEBUG
  Serial.print("status: ");
  Serial.print(stat, HEX);
  Serial.println();
#endif

  sht.requestData(); // Initial request queued up 
}


void loop()
{
  if (sClock::hasIntervalPassed(lastLoopTime, SENSOR_SHT85_MS)) {
    if (sht.dataReady())
    {
      sht.readData();
      temperature = sht.getTemperature(); // TODO use raw version https://github.com/RobTillaart/SHT85
      humidity = sht.getHumidity(); // TODO use raw version https://github.com/RobTillaart/SHT85
      // Note, not smoothing the data as it seems fairly stable and is float rather than bits anyway
  #ifdef SENSOR_SHT85_DEBUG
      Serial.print(temperature, 1);
      Serial.print("°C\t");
      Serial.print(humidity, 1);
      Serial.println("%");
  #endif
      // Note only set next delay and request more Data if was dataReady
      sht.requestData(); // Request next one
      lastLoopTime = sClock::getTime(); 
    }
  }
}
} // namespace sSHT85

#endif WANT_SENSOR_SHT85

//  -- END OF FILE --

