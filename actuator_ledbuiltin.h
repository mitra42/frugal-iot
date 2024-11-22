#ifndef ACTUATOR_LEDBUILTIN_H
#define ACTUATOR_LEDBUILTIN_H

/* Configuration options
 * Optional: ACTUATOR_LEDBUILTIN_DEBUG ACTUATOR_LEDBUILTIN_PIN - defaults
*/

#include "actuator_digital.h" // for class Actuator_Digital

#define ACTUATOR_LEDBUILTIN_TOPIC "ledbuiltin"

class Actuator_Ledbuiltin : public Actuator_Digital {
  public: 
    Actuator_Ledbuiltin(const uint8_t p);
    virtual void act();
};

namespace aLedbuiltin {
extern Actuator_Ledbuiltin actuator_ledbuiltin;

void setup();
void loop();
} // namespace aLedbuiltin
#endif // ACTUATOR_LEDBUILTIN_H