/* Frugal-IoT - system_group 
 * 
 * System_Group is a collection of System_Base (which could include other System_Group) and the 
 * main purpose of this class is to allow easy looping through them.
 *
 */

#ifndef SYSTEM_GROUP_H
#define SYSTEM_GROUP_H

#include "system/base.h"
#include "system/io.h"
#include "system/message.h"

class System_Group : public System_Base {
  public:
    std::vector<System_Base*> group;
    System_Group(const char * const id, const char * const name);
    void setup();
    void setup_after_wifi();
    void setup_after_mqtt();
    System_Base* add(System_Base* fb);
    void dispatch(System_Message &msg) override;
    void discover() override;
    void prepare() override;
    void recover() override;
    void loop() override;
    void periodically() override;
    void infrequently() override;
    void captiveLines(AsyncResponseStream* response) override;
  private:
    void forEach(const char* fnName, void (System_Base::*fn)());
};

#endif //SYSTEM_GROUP_H
