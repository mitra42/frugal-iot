/* Frugal-IoT - system_group 
 * 
 * System_Group is a collection of System_Base (which could include other System_Group) and the 
 * main purpose of this class is to allow easy looping through them.
 *
 */

#ifndef SYSTEM_GROUP_H
#define SYSTEM_GROUP_H

#include "system_base.h"

class System_Group : public System_Base {
  public:
    std::vector<System_Base*> group;
    System_Group(const char * const id, const char * const name);
    void setup();
    void setup_after_wifi();
    void setup_after_mqtt();
    void add(System_Base* fb);
    void dispatchTwig(const String &topicActuatorId, const String &topicLeaf, const String &payload, bool isSet); 
    void dispatchPath(const String &topicPath, const String &payload) override; // Only currently relevant on controls
    void discover() override;
    void loop() override;
    void periodically() override;
    void infrequently() override;
    void captiveLines(AsyncResponseStream* response) override; 
};

#endif //SYSTEM_GROUP_H
