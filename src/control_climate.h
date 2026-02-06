
#include "control.h"

/* Control_Climate: dual-channel hysteresis for temperature + humidity */

class Control_Climate : public Control {
  public:
    Control_Climate(const char* const id, const char* const name,
      float temp_setpoint, float temp_hysteresis,
      float humidity_setpoint, float humidity_hysteresis);
    void act() override;
    #ifdef CONTROL_CLIMATE_DEBUG
      void debug(const char* const where);
    #endif //CONTROL_CLIMATE_DEBUG
};
