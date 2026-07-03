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
    void infrequently() override;
  private: 
    uint8_t timer_index;
    String projectTopic;
    void quickAdvertise();
};


#endif // SYSTEM_DISCOVERY_H
