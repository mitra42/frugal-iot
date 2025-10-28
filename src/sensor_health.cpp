/*  Frugal IoT - Health sensor 
 * 
 *  This will be an aggregation of health fields 
 * 
 * See https://github.com/mitra42/frugal-iot/issues/116 (WiFi strength)
 * Several other issues as well
*/

#include "sensor_health.h"
#include "system_frugal.h"

Sensor_health::Sensor_health(const char* const id, const char* const name) 
: Sensor(id, name, true),
  wifistrength(new OUTuint16(id, "wifistrength", "WiFi Strength", 0, 0, 5, "#000000", false)),
  wifissid(new OUTtext(id, "wifibars", "WiFi SSID", "" ))
{
  outputs.push_back(wifistrength);
  outputs.push_back(wifissid);
}

//void setup() -> Sensor

void Sensor_health::readValidateConvertSet() {
  // TODO-116 get wifi strenght and SSID
  // 4 bars for -55 dBm or higher, 3 bars for -56 to -66 dBm, 2 bars for -67 to -77 dBm, 1 bar for -78 to -88 dBm, and 0 bars for -89 dBm or lower
  wifistrength->set(frugal_iot.wifi->bars());
  wifissid->set(frugal_iot.wifi->SSID());
}