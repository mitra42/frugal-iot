/* Frugal IoT - LoRa controller 
 *
 *  TODO - Its a work in progress - see https://github.com/mitra42/frugal-iot/issues/137
 * 
 */

#include "_settings.h"

#ifdef SYSTEM_LORA_WANT
#include "system_lora.h"

class System_LoRa public Frugal_Base {
    // This class is intended to manage the LoRa system functionality.
public:
    SystemLoRa();
    void setup();
    void infrequent();
    void periodic();
    void frequent();
};

#endif // SYSTEM_LORA_WANT