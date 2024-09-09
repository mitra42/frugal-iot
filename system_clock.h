#ifndef SYSTEM_CLOCK_H
#define SYSTEM_CLOCK_H

#include <Arduino.h>

namespace sClock {
    void init();
    unsigned long getTime();
    bool hasIntervalPassed(unsigned long lastTime, unsigned long interval);
} // namespace sClock
#endif // SYSTEM_CLOCK_H