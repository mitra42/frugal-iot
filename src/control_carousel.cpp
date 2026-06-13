/* 
 * Control_Carousel defines a singleton that was designed to manage a list of Control_Oled displays and cycle between them.
 *
 * It may be more generally useful, as long as the controls in the list can be enabled or disabled for 
 * example I could see it used for managing different heating regimes. 
 *
 * It consists of a vector of "controls" 
 * An index as to which is "selected"
 * An input, "select" that is expected to be tied to carousel/select/cycle=1, typically, based on a button but could also be 
 * driven programatically - e.g. on a timer or based on other factors e.g. with carousel/select=5
 * 
 * When "select" changes, the previous one is disabled, the new one enabled and "selected" updated
*/
#include "control_carousel.h"
#include "Frugal-IoT.h"

Control_Carousel::Control_Carousel(const char* name)
  : Control("carousel", name, std::vector<IN*>{}, std::vector<OUT*>{}),
    selected(0),
    select(new INuint16("carousel", "select", "Select", 0, 0, 0, "#000000", true))
{
  inputs.push_back(select);
}

void Control_Carousel::setup() {
  if (controls.size() > 0) {
    select->max = (uint16_t)(controls.size() - 1);
  }
  Control::setup();
}

// FLAG direct module to module interaction not via messages
void Control_Carousel::act() {
  if (select->value != selected) {
    controls[selected]->enabled = false;
    selected = select->value;
    controls[selected]->enabled = true;
    controls[selected]->act();
  }
}
