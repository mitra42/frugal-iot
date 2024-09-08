#ifndef LOGIC_CLOCK_H
#define LOGIC_CLOCK_H

#include <Arduino.h>

namespace Clock {
    void init();
    unsigned long getTime();
    bool hasIntervalPassed(unsigned long lastTime, unsigned long interval);
} // namespace Clock
#endif // LOGIC_CLOCK_H