// Deep Sleep issues: none - doneFullAdvertise is a plain member and so lost, but System_Power::recover() sets it on a deep-sleep wake, which System_Power::setup() detects with wake_count.
#ifndef SYSTEM_DISCOVERY_H
#define SYSTEM_DISCOVERY_H

#include "system/base.h"
#include "system/io.h"

class System_Discovery : public System_Base {
  public:
    bool doneFullAdvertise = false;
    System_Discovery();
    void fullAdvertise();
    void setup();
    void loop() override;
    void infrequently() override;
  private: 
    uint8_t timer_index;
    String projectTopic;
    void quickAdvertise();
};


#endif // SYSTEM_DISCOVERY_H
