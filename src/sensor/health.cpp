/*  Frugal IoT - Health sensor 
 * 
 *  This will be an aggregation of health fields 
 * 
 * See https://github.com/mitra42/frugal-iot/issues/116 (WiFi strength)
 * Several other issues as well
*/

#include "sensor/health.h"
#include "system/frugal.h"

Sensor_Health::Sensor_Health(const char* const id, const char* const name)
: Sensor(id, name, true),
  wifibars(new OUTuint16(id, "wifibars", "WiFi Strength", 0, IO_DEFAULTS(health_wifibars), false)),
  wifissid(new OUTtext(id, "wifissid", "WiFi SSID", "" ))
{
  outputs.push_back(wifibars);
  outputs.push_back(wifissid);
}

//void setup() -> Sensor

/* Only while connected. Before that WiFi.SSID() is "" - the initial value, so nothing was ever
 * sent - and WiFi.RSSI() is 0, which rssi_to_bars() reports as 5 bars, full strength.
 */
void Sensor_Health::readValidateConvertSet() {
  if (frugal_iot.wifi->connected()) {
    wifibars->set(frugal_iot.wifi->bars());
    wifissid->set(frugal_iot.wifi->SSID());
  }
}

/* Read again as WiFi connects. periodically() runs on the first loop() after boot - under
 * Power_Deep that is every wake - which is before WiFi has connected, so without this a sleeping
 * node would never report its WiFi at all.
 */
void Sensor_Health::loop() {
  const bool connected = frugal_iot.wifi->connected();
  if (connected && !wasConnected) {
    readValidateConvertSet();
  }
  wasConnected = connected;
}