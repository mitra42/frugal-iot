#include "system_clock.h"

namespace sClock {
    void init() {

    }

    unsigned long getTime() {
        return millis();
    }

    bool hasIntervalPassed(unsigned long lastTime, unsigned long interval) {
        unsigned long currentTime = getTime();
        return (currentTime - lastTime >= interval);
    }
} // namespace sClock