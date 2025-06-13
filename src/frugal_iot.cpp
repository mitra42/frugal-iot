/* Frugal-Iot main controller class 
 * 
 * One class to rule them all .... 
 * You should be able to get to any data structure from here.
 * 
 * DRAFT: This is a first-cut, it will be incrementally added to while (hopefully) maintaining 
 * working code, though potentially at the risk of inconsistency 
 * i.e. some things will be handled the OLD way, and some the new - through here.
 * 
 * See https://github.com/mitra42/frugal-iot/issues/141 for discussion and task list
 * 
 * QUICK TODOs - remove as done
 * 
 * Incremental tasks
 *  Move material from main.cpp::setup to Frugal_IoT::setup
 */


#include "_settings.h" // Note - ideally shouldnt be dependent on anything here, or at least not in _local.h
#include "frugal_iot.h"

Frugal_Group::Frugal_Group(const char * const id, const char * const name)
: Frugal_Base(id, name)
{}

void Frugal_Group::add(Frugal_Base* fb) {
  group.push_back(fb);
}

Frugal_IoT::Frugal_IoT()
: Frugal_Group("frugal_iot", "Frugal_IoT"),
  actuators(new Frugal_Group("actuators", "Actuators")),
  sensors(new Frugal_Group("sensors", "Sensors")),
  controls(new Frugal_Group("controls", "Controls"))
{
  add(actuators);
  add(sensors);
  add(controls);
}

// TODO-141 move most of main.cpp::setup to here, all non-app stuff
void Frugal_Group::setup() {
  for (Frugal_Base* fb: group) {
    //TODO-141 move name to FB // Serial.print("Setting up Actuator:"); Serial.print(a->name); Serial.print(" id="); Serial.println(a->id);
    fb->setup();
  }
}

void Frugal_Group::dispatchTwig(const String &topicTwig, const String &payload, bool isSet) {
  // topic Twig  <actuatorId>/<ioID> or  <actuatorId>/set/<ioID> or <actuatorId>/set/<ioID>/<config>
  // This came from Actuators - not checked on Sensors or Control yet
  uint8_t slashPos = topicTwig.indexOf('/'); // Find the position of the slash
  if (slashPos != -1) {
    String id = topicTwig.substring(0, slashPos);       // Extract the part before the slash
    String topicLeaf = topicTwig.substring(slashPos + 1);      // Extract the part after the slash
    for (Frugal_Base* fb: group) {
      fb->dispatchTwig(id, topicLeaf, payload, isSet);
    }
  } else {
    Serial.println("No slash found in topic: " + topicTwig);
  }
}
void Frugal_Group::dispatchPath(const String &topicPath, const String &payload) {
  for (Frugal_Base* fb: group) {
    fb->dispatchPath(topicPath, payload);
  }
}

String Frugal_Group::advertisement() {
  String ad = String();
  for (Frugal_Base* fb: group) {
    ad += (fb->advertisement());
  }
  return ad;
}

void Frugal_Group::periodically() {
  for (Frugal_Base* fb: group) {
    fb->periodically();
  }
}



