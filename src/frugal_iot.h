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
 */

 #ifndef FRUGAL_IOT_H
 #define FRUGAL_IOT_H

#include "_base.h"
#include <vector>

class Frugal_Group : public Frugal_Base {
  public:
    std::vector<Frugal_Base*> group;
    Frugal_Group(const char * const id, const char * const name);
    void setup();
    void add(Frugal_Base* fb);
    void dispatchTwig(const String &topicTwig, const String &payload, bool isSet);
    void dispatchPath(const String &topicPath, const String &payload); // Only currently relevant on controls
    String advertisement();
    void periodically();
};
class Frugal_IoT : public Frugal_Group {
  public:
    Frugal_Group* actuators;
    Frugal_Group* sensors;
    Frugal_Group* controls;
    Frugal_IoT();
};

extern Frugal_IoT frugal_iot;

 #endif // FRUGAL_IOT_H
