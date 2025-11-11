/*  Frugal IoT - Health sensor 
 * 
 *  This will be an aggregation of health fields 
 * 
 * See https://github.com/mitra42/frugal-iot/issues/116 (WiFi strength)
 * Several other issues as well
*/

#include "sensor_health.h"
#include "system_frugal.h"

Sensor_Health::Sensor_Health(const char* const id, const char* const name) 
: Sensor(id, name, true),
  wifibars(new OUTuint16(id, "wifibars", "WiFi Strength", 0, 0, 5, "#000000", false)),
  wifissid(new OUTtext(id, "wifissid", "WiFi SSID", "" ))
{
  outputs.push_back(wifibars);
  outputs.push_back(wifissid);
}

//void setup() -> Sensor

void Sensor_Health::readValidateConvertSet() {
  // TODO-116 get wifi strenght and SSID
  // 4 bars for -55 dBm or higher, 3 bars for -56 to -66 dBm, 2 bars for -67 to -77 dBm, 1 bar for -78 to -88 dBm, and 0 bars for -89 dBm or lower
  wifibars->set(frugal_iot.wifi->bars());
  wifissid->set(frugal_iot.wifi->SSID());
}