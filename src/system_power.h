/* Frugal IoT - System Power - control power managemwent 
 * 
 */
#ifndef SYSTEM_POWER_H
#define SYSTEM_POWER_H

#include "_settings.h"
#include "_base.h"

#define SYSTEM_POWER_DEBUG

class System_Power : public Frugal_Base {
  public:
    unsigned long nextSleepTime = 0;
    System_Power();
    void setup();
    void prepareForDeepSleep();
    void recoverFromDeepSleep();
    void prepareForLightSleep();
    void recoverFromLightSleep();
    bool maybeSleep();
};

extern System_Power* powerController;

#endif // SYSTEM_POWER_H