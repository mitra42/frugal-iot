#include "Frugal-IoT.h"
#include "system_buttons.h"

System_Buttons::System_Buttons (const char* const id, const char * const name) 
: System_Group(id, name), outputs(std::vector<OUT*> { })
{
};
void System_Buttons::setup() {
  for (auto &output : outputs) {
      output->setup();
  }
  readConfigFromFS(); // Reads config (hostname) and passes to our dispatchTwig - should be after inputs and outputs setup (probably)
}

void System_Buttons::dispatchTwig(const String &topicControlId, const String &topicTwig, const String &payload, bool isSet) {
  // Note intentionally not writing to disk as messages like clicks should not be retained
  // TODO-23 if deep sleeping it could be a problem, that not remembering cycle
  if (topicControlId == id) { // matches this control
    for (auto &output : outputs) {
      output->dispatchLeaf(topicTwig, payload, isSet); // Will send value if wiredPath changed
    }
    System_Base::dispatchTwig(topicControlId, topicTwig, payload, isSet);
  }
}
void System_Buttons::discover() {
  for (auto &output : outputs) {
    output->discover();
  }
}
//TODO-23 will need to write things like cycle state to disk 