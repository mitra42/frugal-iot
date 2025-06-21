#ifndef SYSTEM_DISCOVERY_H
#define SYSTEM_DISCOVERY_H

#include "system_base.h"

class System_Discovery : public System_Base {
  public:
    String *topicPrefix;
    bool doneFullAdvertise = false;
    System_Discovery();
    void fullAdvertise();
    void setup_after_mqtt();
    void infrequently();
  private: 
    unsigned long nextLoopTime = 0; // sleepSafeMillis
    //TODO Optimization - should these be String & instead of String *
    // projectTopic - gets 30592; 332252 *projectTopic 30584 / 332220
    String *projectTopic;
    String *advertiseTopic;
    void quickAdvertise();
};


#endif // SYSTEM_DISCOVERY_H
