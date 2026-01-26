/* Frugal IoT - watchdog and monitorying */
#ifndef SYSTEM_WATCHDOG_H
#define SYSTEM_WATCHDOG_H

#include "_settings.h"
#include "system_base.h"

class System_Watchdog : public System_Base {
  public:
    System_Watchdog();
  protected:
  private:
    void setup() override;
    void loop() override;
    #if defined(SYSTEM_MEMORY_DEBUG)
      void infrequently() override;
    #endif
    uint8_t timer_index;

};

#endif // SYSTEM_WATCHDOG_H
