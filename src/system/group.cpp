/* Frugal-IoT - system_group 
 * 
 * System_Group is a collection of System_Base (which could include other System_Group) and the 
 * main purpose of this class is to allow easy looping through them.
 *
 */

#include "system/group.h"

System_Group::System_Group(const char * const id, const char * const name)
: System_Base(id, name)
{}

System_Base* System_Group::add(System_Base* fb) {
  group.push_back(fb);
  return fb;
}


// Loops over the members of the group calling fn on each, printing fnName + the group's and
// member's id when SYSTEM_GROUP_DEBUG is defined.
void System_Group::forEach(const char* fnName, void (System_Base::*fn)()) {
  for (System_Base* fb: group) {
    #ifdef SYSTEM_GROUP_DEBUG
      Serial.printf("%s: %s/%s\n", fnName, id, fb->id);
    #endif
    (fb->*fn)();
  }
}

void System_Group::setup()        { forEach("setup", &System_Base::setup); }
void System_Group::discover()     { forEach("discover", &System_Base::discover); }
void System_Group::prepare()      { forEach("prepare", &System_Base::prepare); }
void System_Group::recover()      { forEach("recover", &System_Base::recover); }
void System_Group::loop()         { forEach("loop", &System_Base::loop); }
void System_Group::periodically() { forEach("periodically", &System_Base::periodically); }
void System_Group::infrequently() { forEach("infrequently", &System_Base::infrequently); }

void System_Group::captiveLines(AsyncResponseStream* response)
  { for (System_Base* fb: group) { fb->captiveLines(response); } }
  
void System_Group::dispatch(System_Message &msg) {
  for (System_Base* fb: group) {
    fb->dispatch(msg);
  }
}

