/* Frugal IoT - LoRa controller 
 *
 *  TODO - Its a work in progress - see https://github.com/mitra42/frugal-iot/issues/137
 * 
 */
#ifndef SYSTEM_LORA_H
#define SYSTEM_LORA_H
#include "_settings.h"

#ifdef SYSTEM_LORA_WANT
#include "_base.h"
#include <LoRa.h> // Note that LoRa is declared globally in <LoRa.h> and used directly in callers.

class System_LoRa : public Frugal_Base {
    // This class is intended to manage the LoRa system functionality.
public:
    System_LoRa();
    void setup();
    void infrequent();
    void periodic();
    void frequent();
    void prepareForSleep();
};

#endif // SYSTEM_LORA_WANT
#endif // SYSTEM_LORA_H