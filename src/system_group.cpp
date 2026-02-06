/* Frugal-IoT - system_group 
 * 
 * System_Group is a collection of System_Base (which could include other System_Group) and the 
 * main purpose of this class is to allow easy looping through them.
 *
 */

#include "system_group.h"

System_Group::System_Group(const char * const id, const char * const name)
: System_Base(id, name)
{}

void System_Group::add(System_Base* fb) {
  group.push_back(fb);
}


void System_Group::setup() {
  for (System_Base* fb: group) {
    #ifdef SYSTEM_FRUGAL_DEBUG
      Serial.println(fb->id);
    #endif
    fb->setup();
  }
}

void System_Group::dispatchTwig(const String &topicSensorId, const String &topicLeaf, const String &payload, bool isSet) {
  for (System_Base* fb: group) {
    fb->dispatchTwig(topicSensorId, topicLeaf, payload, isSet);
  }
};
void System_Group::discover() {
  for (System_Base* fb: group) {
    fb->discover();
  }
}
// These just loop over the members of the group 
void System_Group::loop() {
  for (System_Base* fb: group) { 
    fb->loop(); 
  } 
}
void System_Group::periodically() { 
  //heap_print(F("Periodic"));
  for (System_Base* fb: group) { 
    #ifdef SYSTEM_MEMORY_DEBUG
      Serial.print(fb->id);  //heap_print(F("Sensor_HT::set"));
    #endif
    fb->periodically();
  } 
  //heap_print(F("/Periodic"));
}
void System_Group::infrequently() { 
  for (System_Base* fb: group) { 
    fb->infrequently(); 
  } 
}
void System_Group::captiveLines(AsyncResponseStream* response) 
  { for (System_Base* fb: group) { fb->captiveLines(response); } }
  
void System_Group::dispatchPath(const String &topicPath, const String &payload) 
  { for (System_Base* fb: group) { fb->dispatchPath(topicPath, payload); } }


