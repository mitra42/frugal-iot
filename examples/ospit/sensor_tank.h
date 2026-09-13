/* Water tank level from a resistive float sender - the gauge OSPIT's irrigation controller uses.
 *
 * Hardware, from the comment in OSPIT's irrigation.lua: a float sender that reads 170 ohm when
 * full and 0 ohm when empty, with an 820 ohm resistor in series to 3v3, read on an ADC pin.
 * So the pin sits at 3.3 * Rs/(Rs+820):
 *
 *   tank full   170 ohm   0.567 V   ~703 counts of a 12-bit ADC at 11db attenuation
 *   tank empty    0 ohm   0.000 V      0 counts
 *   sender disconnected - the divider's lower leg is gone, so the pin is pulled to 3v3, ~4095
 *
 * That last row is the point of this class. OSPIT's live code is:
 *
 *     local tankgaugeadc = ADCmeasure(4, 2)
 *     if tankgaugeadc > 4000 then tankgauge = 0 end
 *     if tankgaugeadc < 3000 then tankgauge = 1 end
 *
 * which is a CONNECTIVITY test, not a level measurement - it never distinguishes a full tank from
 * an empty one, only "sender present" from "sender open circuit", and it reports the open-circuit
 * case as 0, i.e. as an empty tank. Since irrigation.lua then tests `tankgauge <= 0`, a node whose
 * tank sender is unplugged refuses to irrigate and looks exactly like a node whose tank is dry.
 * (An earlier percentage version is commented out in that file; it never ran, and it read a
 * different ADC channel than the one it configured.)
 *
 * Here those two states are kept apart, which is the whole reason for the class:
 *   - no sender fitted / cable broken -> validate() fails -> publishes "nan" -> isValid() false
 *   - a real level                    -> 0..100 %
 * Control_Irrigation can then treat "no tank sensor" as "do not let the tank block irrigation"
 * while still treating a genuine 0% as "stop", which OSPIT cannot express.
 *
 * Calibrate for your own sender by measuring the raw counts at both ends and setting:
 *   SENSOR_TANK_RAW_EMPTY         (0)     counts with the tank empty
 *   SENSOR_TANK_RAW_FULL          (703)   counts with the tank full
 *   SENSOR_TANK_RAW_DISCONNECTED  (3000)  at or above this, the sender is assumed missing
 * The default disconnect threshold is the bottom of OSPIT's 3000..4000 dead band, which is far
 * above any reading a connected sender of this type can produce.
 *
 * Published as <id>/<id>, e.g. "tank/tank", per Sensor_Float's one-output convention.
 */

#ifndef SENSOR_TANK_H
#define SENSOR_TANK_H

#include "sensor/analog.h"

#ifndef SENSOR_TANK_RAW_EMPTY
  #define SENSOR_TANK_RAW_EMPTY 0
#endif
#ifndef SENSOR_TANK_RAW_FULL
  #define SENSOR_TANK_RAW_FULL 703
#endif
#ifndef SENSOR_TANK_RAW_DISCONNECTED
  #define SENSOR_TANK_RAW_DISCONNECTED 3000
#endif

class Sensor_Tank : public Sensor_Analog {
  public:
    Sensor_Tank(const char* const id, const char* const name, uint8_t pin, bool retain,
                int raw_empty = SENSOR_TANK_RAW_EMPTY,
                int raw_full = SENSOR_TANK_RAW_FULL,
                int raw_disconnected = SENSOR_TANK_RAW_DISCONNECTED);
  protected:
    int raw_disconnected;
    bool validate(int v) override;
    float convert(int v) override;
};

#endif // SENSOR_TANK_H
