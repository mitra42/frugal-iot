#ifndef SENSOR_SHT85_H
#define SENSOR_SHT85_H

/* Configuration options
 * Required: SENSOR_SHT85_DEVICE, SENSOR_SHT85_ADDRESS, SENSOR_SHT85_MS
 * Optional: SENSOR_SHT85_DEBUG
*/

namespace sSHT85 { // TODO 19D review if need namespace once have class 
void setup();
void loop();
} // namespace sSHT85
#endif // SENSOR_SHT85_H
