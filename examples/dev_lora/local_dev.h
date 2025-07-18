#ifndef LOCAL_DEV_H
#define LOCAL_DEV_H

#include <Arduino.h>
#include "system_base.h"

namespace localDev {

void setup();
void periodically(); // This is called once per period, e.g. every 10 seconds
void infrequently(); // This is called at same rate as perioic but needs to check time.
void loop(); // This is called rapidly - every loop and needs to check time.
} // namespace localDev

#endif //LOCAL_DEV_H

