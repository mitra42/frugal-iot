#ifndef SYSTEM_BUTTONS_H
#define SYSTEM_BUTTONS_H

#include "system_base.h"
#include "system_io.h"
#include "system_group.h"

class System_Buttons : public System_Group {
  public:
    std::vector<OUT*> outputs; // Vector of outputs
    System_Buttons(const char* const id, const char* const name);
  protected:
    void setup();
    void dispatch(System_Message &msg) override;
    void discover();
};

#endif // SYSTEM_BUTTONS_H