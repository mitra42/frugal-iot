#ifndef SYSTEM_CLOCK_H
#define SYSTEM_CLOCK_H

#include <Arduino.h>

namespace xClock {
    void init();
    unsigned long getTime();
    bool hasIntervalPassed(unsigned long lastTime, unsigned long interval);
} // namespace xClock
#endif // SYSTEM_CLOCK_H