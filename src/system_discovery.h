#ifndef SYSTEM_DISCOVERY_H
#define SYSTEM_DISCOVERY_H

#include "system_base.h"

class System_Discovery : public System_Base {
  public:
    bool doneFullAdvertise = false;
    System_Discovery();
    void fullAdvertise();
    void setup();
    void infrequently() override;
  private: 
    unsigned long nextLoopTime = 0; // sleepSafeMillis
    String *projectTopic;
    String *advertiseTopic;
    void quickAdvertise();
};


#endif // SYSTEM_DISCOVERY_H
