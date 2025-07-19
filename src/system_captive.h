/* Frugal IoT - captive portal
 * 
 * This is a trivial captive portal, that allows for configuration etc. 
 * 
 * Its main use is for setting the WiFi settings.
 * 
 * BUT .... this is extensible, at some point we might start doing any or all of the following.
 * - displaying current data
 * - informing about version, configured devices and their parameters
 * - downloading saved data files
 * 
 * Its based on the example that comes with ESPAsyncWebServer 
 * https://github.com/ESP32Async/ESPAsyncWebServer/blob/main/examples/CaptivePortal/CaptivePortal.ino
 */
#ifndef SYSTEM_CAPTIVE_H
#define SYSTEM_CAPTIVE_H

#include "system_base.h"

class System_Captive : public System_Base {
  public:
    System_Captive();
    void setup() override;
    void loop() override;
};

#endif // SYSTEM_CAPTIVE_H
