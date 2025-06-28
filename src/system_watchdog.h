/* Frugal IoT - watchdog and monitorying */
#ifndef SYSTEM_WATCHDOG_H
#define SYSTEM_WATCHDOG_H

#include "_settings.h"
#include "system_base.h"

class System_Watchdog : public System_Base {
  public:
    System_Watchdog();
    void setup();
    void infrequently();
};

#endif // SYSTEM_WATCHDOG_H
