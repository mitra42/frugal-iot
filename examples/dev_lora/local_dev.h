#ifndef LOCAL_DEV_H
#define LOCAL_DEV_H

#include <Arduino.h>
#include "system_base.h"

namespace localDev {

void setup() override;
void periodically() override; // This is called once per period, e.g. every 10 seconds
void infrequently() override; // This is called at same rate as perioic but needs to check time.
void loop() override; // This is called rapidly - every loop and needs to check time.
} // namespace localDev

#endif //LOCAL_DEV_H

