#ifndef SENSOR_SHT85_H
#define SENSOR_SHT85_H

/* Configuration options
 * Required: SENSOR_SHT85_DEVICE, SENSOR_SHT85_ADDRESS, SENSOR_SHT85_MS
 * Optional: SENSOR_SHT85_DEBUG
*/


namespace sSHT85 {
#ifndef SENSOR_SHT85_ADDRESS_ARRAY
extern float temperature;
extern float humidity;
#else
extern float temperature[SENSOR_SHT85_COUNT];
extern float humidity[SENSOR_SHT85_COUNT];
#endif // SENSOR_SHT85_ADDRESS_ARRAY

void setup();
void loop();
} // namespace sSHT85
#endif // SENSOR_SHT85_H
