#include "logic_clock.h"

namespace Clock {
    void init() {

    }

    unsigned long getTime() {
        return millis();
    }

    bool hasIntervalPassed(unsigned long lastTime, unsigned long interval) {
        unsigned long currentTime = getTime();
        return (currentTime - lastTime >= interval);
    }
} // namespace Clock